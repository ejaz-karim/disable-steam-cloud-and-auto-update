#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>
#include "steam-nocloud-noupdates/cloud_disable.hpp"
#include "steam-nocloud-noupdates/utility.hpp"

using namespace std;

// ---------------------------------------------------------------------------
// A robust parser and normalizer for messily-formatted sharedconfig.vdf files.
//
// Real Steam configs are rarely clean. Some hold the Steam settings inside a
// UserRoamingConfigStore block; some also contain stray "Software" blocks (a
// duplicate) that Steam wrote as siblings - nested inside the roam, or even at
// top level. We must NEVER duplicate the Steam data again.
//
// What we do:
//   1. Parse the whole file into a tree of blocks (quote-aware, so braces
//      inside FriendsUIJSON values do not confuse us).
//   2. Locate ALL real "Steam" blocks (real = a block, not a "Steam" "1" leaf).
//      The canonical one is preferred under UserRoamingConfigStore, then under
//      a Software block, then the first one found.
//   3. Merge unique settings from every stray Steam block into the canonical
//      one and drop the strays (fixes Steam-in-Steam, split top/bottom files).
//   4. Drop stray "apps" blocks that are not inside the canonical Steam.
//   5. Force the OUTER "CloudEnabled" (a direct child of Steam) to "0" and
//      rebuild the per-game "apps" block from the ACF ids, each "0".
//   6. Prune container blocks (valve/Software) that became empty.
//   7. Emit ONE canonical structure:
//
//        UserRoamingConfigStore
//        {
//            Software
//            {
//                valve
//                {
//                    Steam
//                    {
//                        CloudEnabled  "0"
//                        ... preserved Steam settings ...
//                        apps { <rebuilt> }
//                    }
//                }
//            }
//            ... preserved root-level keys (JSClientStorage, PlaySoundOnToast) ...
//        }
//
//   8. If the input has no UserRoamingConfigStore, one is built and all
//      preserved content is folded in (Steam-level settings such as friendsui,
//      SurveyDate go inside Steam; the rest stays at root level).
//
// Everything we don't explicitly touch is preserved verbatim, so indentation,
// EOL style, spacing, comments and JSON braces survive unchanged.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Small text helpers.
// ---------------------------------------------------------------------------
namespace
{

// Trim leading/trailing whitespace.
string trimS(const string &s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == string::npos)
        return "";
    return s.substr(b, e - b + 1);
}

// Leading whitespace of a line.
string indentOf(const string &s)
{
    size_t b = s.find_first_not_of(" \t");
    if (b == string::npos)
        return s;
    return s.substr(0, b);
}

// Read a quoted string starting at s[i] ('"'). Advances i past the closing
// quote and returns the DECODED contents (\\" becomes ", \\\\ becomes \\).
// Sets `terminated` to false if the closing quote was missing (end of line /
// input reached) - the caller may then close it to keep the output valid.
string readQuoted(const string &s, size_t &i, bool &terminated)
{
    size_t q1 = i; // opening quote
    size_t q2 = q1 + 1;
    while (q2 < s.size())
    {
        if (s[q2] == '\\')
        {
            q2 += 2;
            continue;
        }
        if (s[q2] == '"')
            break;
        ++q2;
    }
    terminated = (q2 < s.size());
    string raw = s.substr(q1 + 1, (q2 < s.size() ? q2 : s.size()) - q1 - 1);
    i = (q2 < s.size()) ? q2 + 1 : s.size();

    string decoded;
    for (size_t k = 0; k < raw.size(); ++k)
    {
        if (raw[k] == '\\' && k + 1 < raw.size())
        {
            decoded += raw[k + 1];
            k++;
        }
        else
        {
            decoded += raw[k];
        }
    }
    return decoded;
}

// Append a closing '"' to a raw span, first neutralizing a trailing backslash
// so the quote is not swallowed as an escape sequence (which would leave the
// string unterminated).
string appendClosingQuote(const string &s)
{
    size_t k = s.size();
    while (k > 0 && s[k - 1] == '\\')
        --k;
    size_t nbs = s.size() - k; // trailing backslash run
    string out = s;
    if (nbs % 2 == 1)
        out += '\\'; // make the run even so the quote actually closes
    out += '"';
    return out;
}

// Case-insensitive comparison.
bool ieq(const string &a, const string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return false;
    return true;
}

// Split into physical lines, keeping each line's trailing newline. Handles LF,
// CRLF, and lone-CR (classic-Mac) line endings so a CR-only file still parses
// into proper lines.
vector<string> splitKeepingNewlines(const string &text)
{
    vector<string> out;
    size_t start = 0;
    while (start < text.size())
    {
        // find_first_of scans once for the nearest line ending (either CR or
        // LF). Two separate find() calls here would be O(n^2) on LF-only or
        // CR-only files, because the absent character is re-scanned to the end
        // of the buffer for every line.
        size_t end = text.find_first_of("\r\n", start);
        if (end == string::npos)
        {
            out.push_back(text.substr(start));
            break;
        }
        // Include a full CRLF pair as one line ending.
        size_t after = end + 1;
        if (text[end] == '\r' && after < text.size() && text[after] == '\n')
            after++;
        out.push_back(text.substr(start, after - start));
        start = after;
    }
    return out;
}

// Detect EOL style so generated lines match the file.
string detectEol(const string &text)
{
    return text.find("\r\n") != string::npos ? "\r\n" : "\n";
}

} // namespace

// ---------------------------------------------------------------------------
// Tokenization.
// ---------------------------------------------------------------------------
namespace
{

enum TokKind
{
    TK_RAW,   // blank / comment / unparsed
    TK_OPEN,  // bare "{"
    TK_CLOSE, // bare "}"
    TK_NAME,  // lone "Name" (potential block header)
    TK_LEAF   // "Name" "Value..."
};

struct Tok
{
    TokKind kind;
    string raw; // original raw line (with trailing newline)
    string key; // decoded first quoted name
};

// Tokenize one physical line (no trailing newline) into one or more tokens.
// Whitespace between tokens is attached to the FOLLOWING token's raw text, so
// rendering all raws reproduces the original line byte-for-byte.
vector<Tok> scanLine(const string &body)
{
    vector<Tok> out;
    size_t n = body.size();
    size_t tokStart = 0; // raw span start for the token being built
    size_t i = 0;

    auto emit = [&](TokKind k, const string &key, size_t end) {
        Tok t;
        t.kind = k;
        t.key = key;
        t.raw = body.substr(tokStart, end - tokStart);
        out.push_back(std::move(t));
        tokStart = end;
    };

    while (i < n)
    {
        char c = body[i];
        if (c == '{' || c == '}')
        {
            emit(c == '{' ? TK_OPEN : TK_CLOSE, "", i + 1);
            i++;
        }
        else if (c == '"')
        {
            bool keyTerm = true;
            string key = readQuoted(body, i, keyTerm);
            // Look ahead: if a quoted value follows, this is a key-value leaf.
            size_t j = i;
            while (j < n && (body[j] == ' ' || body[j] == '\t' || body[j] == '\r'))
                j++;
            if (j < n && body[j] == '"')
            {
                bool valTerm = true;
                i = j;
                readQuoted(body, i, valTerm); // value (discarded; raw preserves it)
                emit(TK_LEAF, key, i);
                if (!valTerm)
                    out.back().raw = appendClosingQuote(out.back().raw);
            }
            else
            {
                emit(TK_NAME, key, j);
                i = j;
                if (!keyTerm)
                    out.back().raw = appendClosingQuote(out.back().raw);
            }
        }
        else
        {
            // Raw run (junk, comments mid-line, etc.) up to the next token.
            size_t j = i;
            while (j < n && body[j] != '"' && body[j] != '{' && body[j] != '}')
                j++;
            if (body.substr(i, j - i).find_first_not_of(" \t\r") != string::npos)
                emit(TK_RAW, "", j);
            // else: pure whitespace; the next token picks it up via tokStart
            i = j;
        }
    }
    if (tokStart < n)
        emit(TK_RAW, "", n); // trailing whitespace
    return out;
}

vector<Tok> tokenize(const vector<string> &physical)
{
    vector<Tok> out;
    out.reserve(physical.size());
    for (const auto &rawLine : physical)
    {
        string body = rawLine;
        string nl; // original line ending, re-attached to the last token
        if (!body.empty() && body.back() == '\n')
        {
            nl = "\n";
            body.pop_back();
            if (!body.empty() && body.back() == '\r')
            {
                nl = "\r\n";
                body.pop_back();
            }
        }
        else if (!body.empty() && body.back() == '\r')
        {
            nl = "\r"; // lone-CR line ending (classic Mac)
            body.pop_back();
        }

        string t = trimS(body);

        // Comment lines (//...) are raw text even if they contain quotes/braces.
        if (t.size() >= 2 && t[0] == '/' && t[1] == '/')
        {
            Tok tk;
            tk.kind = TK_RAW;
            tk.key = "";
            tk.raw = rawLine;
            out.push_back(std::move(tk));
            continue;
        }

        vector<Tok> line = scanLine(body);
        if (!line.empty())
        {
            line.back().raw += nl;
            for (auto &tk : line)
                out.push_back(std::move(tk));
        }
        else
        {
            Tok tk;
            tk.kind = TK_RAW;
            tk.key = "";
            tk.raw = rawLine; // blank line, preserved verbatim
            out.push_back(std::move(tk));
        }
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Tree representation and rendering.
// ---------------------------------------------------------------------------
struct Seg
{
    bool isBlock = false;   // has { ... } children
    string name;            // decoded name ("" for raw/text)
    string header;          // verbatim intro line(s) for the node
    vector<Seg> kids;       // children (when isBlock)
    string close;           // verbatim closing "}" line (when isBlock)
    bool generated = false; // produced by us; render genText directly
    string genText;         // verbatim text of a generated node
    bool drop = false;      // marked for removal (stray Steam/apps/empty container)
};

// Render a Seg and its children back to text.
static string renderSeg(const Seg &s)
{
    if (s.drop)
        return "";
    if (s.generated)
        return s.genText;
    string o = s.header;
    if (s.isBlock)
    {
        for (const auto &k : s.kids)
            o += renderSeg(k);
        if (!s.close.empty())
            o += s.close;
        else
        {
            // Truncated input: the block's closing brace was never parsed.
            // Close it ourselves so the output stays balanced.
            string eol = "\n";
            if (s.header.find("\r\n") != string::npos)
                eol = "\r\n";
            o += indentOf(s.header) + "}" + eol;
        }
    }
    return o;
}

// ---------------------------------------------------------------------------
// Parsing: convert tokens to a tree of Seg blocks.
// ---------------------------------------------------------------------------
namespace
{

// Pathological nesting (thousands of nested blocks) must not overflow the
// call stack or blow up the output with O(depth^2) indentation. Beyond this
// depth we stop building blocks and flatten the remaining structure into raw
// text. Real Steam configs are <10 deep, so this is far above any real file.
const int MAX_DEPTH = 2048;
// Cap on relative indentation preserved during re-indent (real configs are
// <10 levels deep; anything deeper is junk and gets clamped).
const size_t MAX_INDENT = 64;

// Parse immediate children starting at `idx`. Consumes until a TK_CLOSE (which
// is left for the caller) or the end. Returns the list of child nodes.
// When `isTop` is true (top-level parse), a stray "}" is treated as raw text
// and preserved, and parsing continues - so truncated files with extra closing
// braces don't cause the rest of the file (e.g. JSClientStorage) to be dropped.
vector<Seg> parseChildren(const vector<Tok> &tokens, size_t &idx, bool isTop, int depth = 0)
{
    vector<Seg> out;
    string rawBuf; // verbatim raw text (blank/comment lines) grouped in one node

    auto flushRaw = [&]() {
        if (!rawBuf.empty())
        {
            Seg r;
            r.header = rawBuf;
            out.push_back(std::move(r));
            rawBuf.clear();
        }
    };

    while (idx < tokens.size())
    {
        const Tok &tk = tokens[idx];

        if (tk.kind == TK_RAW)
        {
            rawBuf += tk.raw;
            idx++;
            continue;
        }
        if (tk.kind == TK_CLOSE)
        {
            if (isTop)
            {
                // Stray closing brace at top level: preserve it and keep going.
                rawBuf += tk.raw;
                idx++;
                continue;
            }
            flushRaw();
            return out; // caller consumes the '}'
        }
        if (tk.kind == TK_OPEN)
        {
            // A stray '{' with no name: keep as raw text.
            rawBuf += tk.raw;
            idx++;
            continue;
        }
        if (tk.kind == TK_LEAF)
        {
            flushRaw();
            Seg leaf;
            leaf.name = tk.key;
            leaf.header = tk.raw;
            out.push_back(std::move(leaf));
            idx++;
            continue;
        }
        // TK_NAME: block header, possibly followed by '{'.
        {
            flushRaw();
            Seg node;
            node.name = tk.key;
            node.header = tk.raw;
            idx++;
            if (idx < tokens.size() && tokens[idx].kind == TK_OPEN)
            {
                if (depth >= MAX_DEPTH)
                {
                    // Too deep: flatten this whole nested block into raw text
                    // (balanced by construction) so recursion and output size
                    // stay bounded. The junk is folded/dropped downstream.
                    string flat = node.header + tokens[idx].raw;
                    idx++;
                    int bal = 1;
                    while (idx < tokens.size() && bal > 0)
                    {
                        if (tokens[idx].kind == TK_OPEN)
                            bal++;
                        else if (tokens[idx].kind == TK_CLOSE)
                            bal--;
                        flat += tokens[idx].raw;
                        idx++;
                    }
                    Seg raw;
                    raw.header = flat;
                    out.push_back(std::move(raw));
                    continue;
                }
                node.isBlock = true;
                node.header += tokens[idx].raw;
                idx++;
                node.kids = parseChildren(tokens, idx, false, depth + 1);
                if (idx < tokens.size() && tokens[idx].kind == TK_CLOSE)
                {
                    node.close = tokens[idx].raw;
                    idx++;
                }
            }
            out.push_back(std::move(node));
        }
    }

    flushRaw();
    return out;
}

// Collect all REAL (block) nodes named 'Steam' (any depth) into `out`.
void collectSteamRecursive(Seg &block, vector<Seg *> &out)
{
    if (block.isBlock && ieq(block.name, "Steam"))
        out.push_back(&block);
    for (auto &k : block.kids)
        collectSteamRecursive(k, out);
}

} // namespace

// ---------------------------------------------------------------------------
// Normalization helpers.
// ---------------------------------------------------------------------------
namespace
{

// True if `s` holds settings that belong directly under Steam.
bool isSteamLevel(const Seg &s)
{
    if (s.isBlock)
        return ieq(s.name, "friendsui") || ieq(s.name, "FriendsUI");
    return ieq(s.name, "SurveyDate") || ieq(s.name, "SurveyDateVersion") ||
           ieq(s.name, "StartMenuShortcutCheck") || ieq(s.name, "DesktopShortcutCheck") ||
           ieq(s.name, "SteamDefaultDialog") || ieq(s.name, "ShowScreenshotManager") ||
           ieq(s.name, "CloudEnabled") || ieq(s.name, "cloudenabled");
}

// True if `s` is one of the top-level store containers Steam writes. A real
// sharedconfig.vdf can carry several stores (UserRoamingConfigStore and
// UserLocalConfigStore) as top-level siblings; when we collapse to a single
// canonical store these wrappers must be UNWRAPPED (their children folded)
// rather than nested inside the rebuilt root.
bool isStoreBlock(const Seg &s)
{
    return s.isBlock && (ieq(s.name, "UserRoamingConfigStore") ||
                         ieq(s.name, "UserLocalConfigStore"));
}

// True if the character is ignorable junk: whitespace or any control char
// (form feed, vertical tab, NUL, DEL, ...). Real VDF never uses these as
// content, so they can be ignored when deciding if text is brace-only junk.
bool isIgnorable(char c)
{
    unsigned char u = (unsigned char)c;
    return u <= 0x20 || u == 0x7f;
}

// True if `text` contains only braces (plus ignorable chars) and at least one
// brace - i.e. a stray-brace run with no real content.
bool isBraceOnly(const string &text)
{
    bool any = false;
    for (char c : text)
    {
        if (isIgnorable(c))
            continue;
        if (c == '{' || c == '}')
        {
            any = true;
            continue;
        }
        return false;
    }
    return any;
}

// True if a raw node is only stray braces / ignorable junk (safe to drop).
bool isStrayBrace(const Seg &s)
{
    if (s.isBlock || !s.name.empty())
        return false;
    for (char c : s.header)
        if (!isIgnorable(c) && c != '{' && c != '}')
            return false;
    return true;
}

// Remove bare "{" / "}" LINES from raw junk text so stray braces from mangled
// files cannot unbalance the output. Real content is untouched and newlines
// between kept lines are preserved (reindent re-adds the file's EOL later).
string stripBraceLines(const string &text)
{
    string out;
    size_t start = 0;
    while (start < text.size())
    {
        // Single scan for the nearest line ending (see splitKeepingNewlines).
        size_t end = text.find_first_of("\r\n", start);

        string line = (end == string::npos) ? text.substr(start) : text.substr(start, end - start);
        size_t nextStart = (end == string::npos) ? text.size() : end + 1;
        if (end != string::npos && text[end] == '\r' && nextStart < text.size() && text[nextStart] == '\n')
            nextStart++;

        // Strip any line made up ONLY of braces (single or runs like }}, {{
        // or }{, with any surrounding control/whitespace junk), but keep blank
        // lines and anything containing real content.
        if (!isBraceOnly(line))
        {
            out += line;
            if (end != string::npos)
                out += '\n';
        }
        if (end == string::npos)
            break;
        start = nextStart;
    }
    return out;
}

// Re-indent a rendered block: the first non-blank line gets `pad`, and every
// other line keeps its indentation RELATIVE to the first line (so nested
// content stays nested). Blank lines are left untouched. Handles LF, CRLF and
// lone-CR line endings so a stray carriage return cannot merge two lines.
string reindent(const string &text, const string &pad, const string &eol)
{
    string out;
    size_t start = 0;
    size_t base = 0;
    bool haveBase = false;
    while (start < text.size())
    {
        // Single scan for the nearest line ending (see splitKeepingNewlines).
        size_t end = text.find_first_of("\r\n", start);

        string line = (end == string::npos) ? text.substr(start) : text.substr(start, end - start);
        size_t nextStart = (end == string::npos) ? text.size() : end + 1;
        if (end != string::npos && text[end] == '\r' && nextStart < text.size() && text[nextStart] == '\n')
            nextStart++; // consume the LF of a CRLF pair

        size_t b = line.find_first_not_of(" \t");
        if (b == string::npos)
        {
            // blank / whitespace-only line: keep as-is
            out += line;
            if (end != string::npos)
                out += eol;
        }
        else
        {
            size_t cur = b;
            if (!haveBase)
            {
                base = cur;
                haveBase = true;
            }
            size_t extra = (cur > base) ? cur - base : 0;
            // Cap pathological relative indentation (a stray deeply-nested
            // block must not blow the output up to O(depth^2) tabs).
            if (extra > MAX_INDENT)
                extra = MAX_INDENT;
            out += pad + string(extra, '\t') + line.substr(b) + eol;
        }
        if (end == string::npos)
            break;
        start = nextStart;
    }
    return out;
}

// Quote-aware, comment-aware check: is `text` both quote-balanced and
// brace-balanced? Used to decide whether preserved junk is safe to fold into
// the output - unbalanced braces or a stray unterminated quote would corrupt
// the file we write. `//` comments are ignored (their quotes/braces are not
// structural), matching how the file is parsed elsewhere.
bool isSafeJunk(const string &text)
{
    int depth = 0;
    bool inString = false;
    for (size_t i = 0; i < text.size(); ++i)
    {
        char c = text[i];
        if (inString)
        {
            if (c == '\\')
            {
                if (i + 1 < text.size())
                    i++;
                continue;
            }
            if (c == '"')
                inString = false;
            continue;
        }
        if (c == '"')
            inString = true;
        else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/')
        {
            while (i + 1 < text.size() && text[i] != '\n' && text[i] != '\r')
                i++;
        }
        else if (c == '{')
            depth++;
        else if (c == '}')
        {
            if (--depth < 0)
                return false;
        }
    }
    return !inString && depth == 0;
}

// Render `top`, sanity-check it (balanced braces and quotes), and re-indent to
// `pad`. Returns "" when the content would corrupt the output. When `strip` is
// true, stray brace-only lines are removed first (raw junk).
string foldJunk(const Seg &top, const string &pad, const string &eol, bool strip)
{
    string rendered = renderSeg(top);
    if (strip)
        rendered = stripBraceLines(rendered);
    // Safety-check AFTER stripping: removing a brace-only line can leave its
    // matching brace stranded on a content line (e.g. "{ / junk }"), so the
    // stripped text must itself be balanced before we fold it in.
    if (!isSafeJunk(rendered))
        return "";
    return reindent(rendered, pad, eol);
}

// Re-indent `node` to sit at `pad`: its header line(s) go to `pad`, and every
// direct child goes one level deeper (recursively). Generated nodes are left
// untouched. This cleans up wonky / over-indented in-place indentation from
// mangled inputs while preserving each node's internal relative structure.
void normalizeTree(Seg &node, const string &pad, const string &eol)
{
    if (node.generated || node.drop)
        return;
    if (node.isBlock)
    {
        node.header = reindent(node.header, pad, eol);
        if (!node.close.empty())
            node.close = reindent(node.close, pad, eol);
        // Cap the absolute indentation as well as the relative one: a
        // pathological nesting depth must not pad deep junk with thousands of
        // tabs. That bloats the flattened string to O(depth * size), which
        // renderSeg then copies once per level up the tree - an O(depth^2)
        // blowup on inputs like a 50k-deep brace nest. Real configs are <10
        // levels deep, so anything past MAX_INDENT is junk and can clamp here.
        string childPad = pad;
        if (childPad.size() < MAX_INDENT)
            childPad += "\t";
        vector<Seg> kept;
        for (auto &k : node.kids)
        {
            if (!k.isBlock && k.name.empty())
            {
                // Raw junk: strip stray brace-only lines so mangled { } runs
                // can't unbalance the output, then drop anything that is still
                // quote/brace-unbalanced (it would corrupt the output when this
                // block is rendered verbatim).
                k.header = stripBraceLines(k.header);
                if (trimS(k.header).empty())
                    continue;
                if (!isSafeJunk(k.header))
                    continue;
            }
            normalizeTree(k, childPad, eol);
            kept.push_back(std::move(k));
        }
        node.kids = std::move(kept);
    }
    else
    {
        node.header = reindent(node.header, pad, eol);
    }
}

// Re-indent every direct child of `block` to `pad` (each child's internal
// relative indentation is preserved). Generated nodes are left alone. Used to
// re-level children that were merged in from a stray block at a different
// depth.
void normalizeChildIndents(Seg &block, const string &pad, const string &eol)
{
    vector<Seg> newKids;
    for (auto &k : block.kids)
    {
        if (k.generated || k.drop)
        {
            newKids.push_back(std::move(k));
            continue;
        }
        Seg raw;
        raw.header = reindent(renderSeg(k), pad, eol);
        newKids.push_back(std::move(raw));
    }
    block.kids = std::move(newKids);
}

// True if `block` has a direct child named `name` (case-insensitive).
bool hasKid(const Seg &block, const string &name)
{
    for (const auto &k : block.kids)
        if (ieq(k.name, name))
            return true;
    return false;
}

// True if `node` is `target` itself or a descendant of it.
bool containsSeg(const Seg &node, const Seg *target)
{
    if (&node == target)
        return true;
    for (const auto &k : node.kids)
        if (containsSeg(k, target))
            return true;
    return false;
}

// First Steam block found when walking below a node that is (or is inside) a
// block named `target`; nullptr if none.
Seg *findSteamUnder(Seg &node, const string &target, bool inside)
{
    if (inside && node.isBlock && ieq(node.name, "Steam"))
        return &node;
    bool childInside = inside || (node.isBlock && ieq(node.name, target));
    for (auto &k : node.kids)
    {
        Seg *f = findSteamUnder(k, target, childInside);
        if (f)
            return f;
    }
    return nullptr;
}

// Pick the canonical Steam block: prefer the real chain
// (Software > valve > Steam), then under a Software block, then under a
// UserRoamingConfigStore, then the first one found.
Seg *pickCanonicalSteam(vector<Seg> &tops, const vector<Seg *> &allSteams)
{
    for (auto &top : tops)
    {
        Seg *f = findSteamUnder(top, "valve", false);
        if (f)
            return f;
    }
    for (auto &top : tops)
    {
        Seg *f = findSteamUnder(top, "Software", false);
        if (f)
            return f;
    }
    for (auto &top : tops)
    {
        Seg *f = findSteamUnder(top, "UserRoamingConfigStore", false);
        if (f)
            return f;
    }
    return allSteams[0];
}

// Mark stray "apps" blocks (not inside the canonical Steam) for removal.
void markStrayApps(Seg &node, const Seg *canonical, bool insideCanonical)
{
    // Never drop an apps block that CONTAINS the canonical Steam (it can only
    // be an ancestor of the canonical when the input was mangled so badly that
    // the canonical ended up nested inside a stray apps block). Dropping it
    // would take the canonical Steam with it and leave no Steam in the output.
    if (node.isBlock && ieq(node.name, "apps") && !insideCanonical
        && !containsSeg(node, canonical))
        node.drop = true;
    bool childInside = insideCanonical || (&node == canonical);
    for (auto &k : node.kids)
        markStrayApps(k, canonical, childInside);
}

// Mark emptied container blocks (valve/Software) for removal, bottom-up.
// Returns true when the node contributes no content anymore.
bool markEmptyContainers(Seg &node)
{
    if (!node.isBlock)
        return node.drop;
    bool allKidsGone = true;
    for (auto &k : node.kids)
        if (!markEmptyContainers(k))
            allKidsGone = false;
    if (allKidsGone && (ieq(node.name, "valve") || ieq(node.name, "Software")))
        node.drop = true;
    return node.drop;
}

// Text of a generated "apps" block whose children sit at indent `ci`.
string buildAppsText(const string &ci, const vector<string> &ids, const string &eol)
{
    string acc = ci + "\"apps\"" + eol;
    acc += ci + "{" + eol;
    for (const auto &id : ids)
    {
        acc += ci + "\t" + id + eol;
        acc += ci + "\t{" + eol;
        acc += ci + "\t\t\"CloudEnabled\"\t\t\"0\"" + eol;
        acc += ci + "\t}" + eol;
    }
    acc += ci + "}" + eol;
    return acc;
}

// Text of a generated "Steam" block whose header sits at indent `pid`.
string buildSteamBlockText(const string &pid, const vector<string> &ids, const string &eol)
{
    string ci = pid + "\t";
    string s = pid + "\"Steam\"" + eol;
    s += pid + "{" + eol;
    s += ci + "\"CloudEnabled\"\t\t\"0\"" + eol;
    s += buildAppsText(ci, ids, eol);
    s += pid + "}" + eol;
    return s;
}

// Force the canonical Steam's global CloudEnabled to "0" and rebuild apps.
void transformCanonical(Seg &steam, const vector<string> &ids, const string &eol)
{
    string steamChildIndent = indentOf(steam.header) + "\t";

    vector<Seg> newKids;
    bool sawOuterCloud = false;
    bool appsDone = false;

    for (auto &k : steam.kids)
    {
        if (ieq(k.name, "apps"))
        {
            // Replace the FIRST node named apps (block or stray leaf) with the
            // rebuilt block; drop any extra duplicate apps nodes.
            if (!appsDone)
            {
                appsDone = true;
                Seg apps;
                apps.name = "apps";
                apps.generated = true;
                apps.genText = buildAppsText(steamChildIndent, ids, eol);
                newKids.push_back(std::move(apps));
            }
            continue;
        }

        if (ieq(k.name, "CloudEnabled") && !k.isBlock)
        {
            // Replace the FIRST non-block CloudEnabled; drop any extras.
            if (!sawOuterCloud)
            {
                sawOuterCloud = true;
                Seg cloud;
                cloud.name = "CloudEnabled";
                cloud.generated = true;
                cloud.genText = steamChildIndent + "\"CloudEnabled\"\t\t\"0\"" + eol;
                newKids.push_back(std::move(cloud));
            }
            continue;
        }

        {
            string rendered = renderSeg(k);
            // Strip stray brace-only lines from raw junk children (so "{ /
            // junk }" can't unbalance the Steam block), then drop anything
            // still unsafe.
            if (!k.isBlock && k.name.empty())
                rendered = stripBraceLines(rendered);
            if (isSafeJunk(rendered))
            {
                if (!k.isBlock && k.name.empty())
                    k.header = rendered;
                newKids.push_back(std::move(k));
            }
        }
    }

    if (!sawOuterCloud)
    {
        Seg cloud;
        cloud.name = "CloudEnabled";
        cloud.generated = true;
        cloud.genText = steamChildIndent + "\"CloudEnabled\"\t\t\"0\"" + eol;
        newKids.insert(newKids.begin(), std::move(cloud));
    }

    if (!appsDone)
    {
        Seg apps;
        apps.name = "apps";
        apps.generated = true;
        apps.genText = buildAppsText(steamChildIndent, ids, eol);
        newKids.push_back(std::move(apps));
    }

    // Put the transformed children back, then re-level preserved children
    // (including any merged in from stray Steam blocks) so the Steam block is
    // clean even when input indents were wonky. (Without this assignment the
    // moved-from kids left behind by the loop above would render stray "}".)
    steam.kids = std::move(newKids);
    normalizeChildIndents(steam, steamChildIndent, eol);
}

// A UserRoamingConfigStore exists but no real Steam block was found: add a
// canonical Steam (CloudEnabled "0" + rebuilt apps) into its Software>valve,
// replacing any "Steam" leaf/block already there. Returns the output text.
string addSteamToRoam(vector<Seg> &tops, const vector<string> &ids, const string &eol)
{
    Seg *roam = nullptr;
    for (auto &top : tops)
        if (top.isBlock && ieq(top.name, "UserRoamingConfigStore"))
        {
            roam = &top;
            break;
        }
    if (!roam)
        return "";

    Seg *software = nullptr;
    for (auto &k : roam->kids)
        if (k.isBlock && ieq(k.name, "Software"))
        {
            software = &k;
            break;
        }
    if (!software)
    {
        Seg s;
        s.name = "Software";
        s.isBlock = true;
        s.header = "\t\"Software\"" + eol + "\t{" + eol;
        s.close = "\t}" + eol;
        roam->kids.insert(roam->kids.begin(), std::move(s));
        software = &roam->kids.front();
    }

    // Drop any stray "Steam" leafs directly under Software (a real Steam block
    // would have been found above, so anything named Steam here is just junk).
    {
        vector<Seg> kept;
        for (auto &k : software->kids)
            if (!ieq(k.name, "Steam"))
                kept.push_back(std::move(k));
        software->kids = std::move(kept);
    }

    Seg *valve = nullptr;
    for (auto &k : software->kids)
        if (k.isBlock && ieq(k.name, "valve"))
        {
            valve = &k;
            break;
        }
    if (!valve)
    {
        Seg v;
        v.name = "valve";
        v.isBlock = true;
        v.header = "\t\t\"valve\"" + eol + "\t\t{" + eol;
        v.close = "\t\t}" + eol;
        software->kids.push_back(std::move(v));
        valve = &software->kids.back();
    }

    // Replace any existing "Steam" child (leaf or block) under valve.
    string pid = indentOf(valve->header) + "\t";
    vector<Seg> kept;
    for (auto &k : valve->kids)
        if (!ieq(k.name, "Steam"))
            kept.push_back(std::move(k));
    valve->kids = std::move(kept);

    Seg steam;
    steam.name = "Steam";
    steam.generated = true;
    steam.genText = buildSteamBlockText(pid, ids, eol);
    valve->kids.push_back(std::move(steam));
    Seg *steamPtr = &valve->kids.back();

    // Stray "apps" directly under the roam are regenerated under Steam.
    for (auto &k : roam->kids)
        if (k.isBlock && ieq(k.name, "apps"))
            k.drop = true;
    // Stray top-level "Software" duplicates.
    for (auto &top : tops)
        if (&top != roam && ieq(top.name, "Software"))
            top.drop = true;

    // Fold stray top-level content the same way buildRoamRoot does: drop
    // brace-only junk and stray CloudEnabled leaves, fold Steam-level settings
    // into the added Steam, and fold everything else into the roam.
    string steamExtra;
    string rootExtra;
    for (auto &top : tops)
    {
        if (&top == roam || top.drop)
            continue;
        if (isStrayBrace(top))
        {
            top.drop = true;
            continue;
        }
        if (!top.isBlock && ieq(top.name, "CloudEnabled"))
        {
            top.drop = true;
            continue;
        }
        if (isStoreBlock(top))
        {
            // Unwrap a second store so its content becomes root siblings.
            for (auto &k : top.kids)
                if (!k.drop)
                    rootExtra += foldJunk(k, "\t", eol, false);
            top.drop = true;
            continue;
        }
        if (isSteamLevel(top))
            steamExtra += foldJunk(top, pid + "\t", eol, false);
        else if (top.isBlock || !top.name.empty())
            rootExtra += foldJunk(top, "\t", eol, false);
        else
            rootExtra += foldJunk(top, "\t", eol, true);
        top.drop = true;
    }

    // Steam-level settings that sit DIRECTLY at the roam's root (real files
    // keep them under Steam, but corrupted ones can scatter them) belong in the
    // added Steam. A stray root-level CloudEnabled leaf is dropped instead,
    // because the generated Steam already provides the global switch. Marking
    // the nodes dropped (rather than erasing them) keeps `software`/`valve`/
    // `steamPtr` valid - renderSeg simply skips dropped children.
    for (auto &k : roam->kids)
    {
        if (k.drop)
            continue;
        if (!k.isBlock && ieq(k.name, "CloudEnabled"))
        {
            k.drop = true;
            continue;
        }
        if (isSteamLevel(k))
        {
            steamExtra += foldJunk(k, pid + "\t", eol, false);
            k.drop = true;
        }
    }

    if (!steamExtra.empty())
    {
        // Insert the folded Steam-level settings just before the added
        // Steam block's closing brace so they land INSIDE the block.
        string close = pid + "}" + eol;
        string text = steamPtr->genText;
        size_t pos = text.rfind(close);
        if (pos != string::npos)
            text.insert(pos, steamExtra);
        else
            text += steamExtra;
        steamPtr->genText = text;
    }
    if (!rootExtra.empty())
    {
        Seg raw;
        raw.header = rootExtra;
        roam->kids.push_back(std::move(raw));
    }

    string outText;
    for (const auto &top : tops)
        if (!top.drop)
            outText += renderSeg(top);
    return outText;
}

// Fold every piece of content under `node` that is NOT on the chain leading to
// the canonical Steam into `out`, unwrapping any store wrappers found on that
// chain. The canonical Steam's own children are handled separately.
void foldNonChainContent(const Seg &node, const Seg *canonical, const string &eol, string &out)
{
    if (&node == canonical)
        return;
    for (const auto &k : node.kids)
    {
        if (k.drop)
            continue;
        if (containsSeg(k, canonical))
        {
            if (k.isBlock && &k != canonical)
                foldNonChainContent(k, canonical, eol, out);
            continue;
        }
        out += foldJunk(k, "\t", eol, false);
    }
}

// True if `roam` reaches the canonical Steam through a store block (a nested
// UserLocalConfigStore / UserRoamingConfigStore) rather than directly through
// the Software/valve chain.
bool storeWrapperOnChain(const Seg &roam, const Seg *canonical)
{
    for (const auto &k : roam.kids)
        if (k.isBlock && isStoreBlock(k) && containsSeg(k, canonical))
            return true;
    return false;
}

// Build a fresh single-root UserRoamingConfigStore. When `canonical` is
// non-null its preserved settings seed the Steam block; otherwise top-level
// Steam-level settings (SurveyDate, friendsui, ...) are folded in as well.
string buildRoamRoot(vector<Seg> &tops, Seg *canonical, const vector<string> &ids, const string &eol)
{
    string ci = "\t\t\t\t";
    string steamExtra;
    if (canonical)
    {
        for (auto &k : canonical->kids)
        {
            if (k.drop)
                continue;
            if (ieq(k.name, "apps"))
                continue;
            if (ieq(k.name, "CloudEnabled") && !k.isBlock)
                continue;
            steamExtra += foldJunk(k, ci, eol, false);
        }
    }

    string rootExtra;
    for (const auto &top : tops)
    {
        if (top.drop)
            continue;
        if (canonical && containsSeg(top, canonical))
        {
            // The store block that owns the canonical Steam. Preserve content
            // OUTSIDE the chain leading to the Steam (JSClientStorage /
            // WebStorage siblings, and any nested store wrapper's extra
            // content), but don't re-emit the chain - the canonical Steam is
            // seeded below.
            foldNonChainContent(top, canonical, eol, rootExtra);
            continue;
        }
        if (isStoreBlock(top))
        {
            // A second store (UserLocalConfigStore / a duplicate roam). Unwrap
            // it so its content becomes root-level siblings instead of a nested
            // store block inside the rebuilt root.
            for (const auto &k : top.kids)
                if (!k.drop)
                    rootExtra += foldJunk(k, "\t", eol, false);
            continue;
        }
        if (ieq(top.name, "apps"))
            continue; // regenerated under Steam
        if (canonical && ieq(top.name, "Software"))
            continue; // stray container
        if (!top.isBlock && ieq(top.name, "CloudEnabled"))
            continue; // canonical block provides the global switch
        if (isStrayBrace(top))
            continue; // drop stray "}" braces from truncated files
        if (isSteamLevel(top))
            steamExtra += foldJunk(top, ci, eol, false);
        else if (top.isBlock || !top.name.empty())
            rootExtra += foldJunk(top, "\t", eol, false);
        else
            rootExtra += foldJunk(top, "\t", eol, true);
    }

    string full;
    full += "\"UserRoamingConfigStore\"" + eol;
    full += "{" + eol;
    full += "\t\"Software\"" + eol;
    full += "\t{" + eol;
    full += "\t\t\"valve\"" + eol;
    full += "\t\t{" + eol;
    full += "\t\t\t\"Steam\"" + eol;
    full += "\t\t\t{" + eol;
    full += ci + "\"CloudEnabled\"\t\t\"0\"" + eol;
    full += buildAppsText(ci, ids, eol);
    full += steamExtra;
    full += "\t\t\t}" + eol; // Steam close
    full += "\t\t}" + eol;   // valve close
    full += "\t}" + eol;     // Software close
    full += rootExtra;
    full += "}" + eol;
    return full;
}

bool writeOutput(const string &path, const string &text, size_t idCount)
{
    ofstream sharedConfigFile(path, ios::trunc | ios::binary);
    if (!sharedConfigFile)
    {
        cerr << "Error: Could not open file " << path << endl;
        return false;
    }
    sharedConfigFile << text;
    sharedConfigFile.close();

    cout << ">Modified: " << path << endl;
    cout << ">Cloud disabled for " << idCount << " games" << endl;
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// CloudDisabler
// ---------------------------------------------------------------------------
CloudDisabler::CloudDisabler() {}

bool CloudDisabler::replaceAppsBlock(const string &sharedConfigPath, const string &sharedConfigText, const string &acfIds)
{
    string eol = detectEol(sharedConfigText);
    vector<string> physical = splitKeepingNewlines(sharedConfigText);
    vector<Tok> tokens = tokenize(physical);

    size_t idx = 0;
    vector<Seg> tops = parseChildren(tokens, idx, true);

    // ---- Normalize in-place indentation to a clean tab scheme so wonky /
    //      over-indented inputs produce clean output. Structural fixes below
    //      (Steam merge, apps rebuild) build on top of this. ----
    for (auto &top : tops)
        normalizeTree(top, "", eol);

    // ---- Parse the ACF ids (already quoted, one per line). ----
    vector<string> ids;
    {
        stringstream ss(acfIds);
        string lid;
        while (getline(ss, lid))
        {
            string s = trimS(lid);
            if (!s.empty())
                ids.push_back(s);
        }
    }

    // ---- Locate ALL real Steam blocks anywhere in the file. ----
    vector<Seg *> allSteams;
    for (auto &top : tops)
        collectSteamRecursive(top, allSteams);

    string outText;

    if (allSteams.empty())
    {
        // No real Steam block anywhere. We must NOT destroy existing data.
        bool hasRoam = false;
        for (const auto &top : tops)
            if (top.isBlock && ieq(top.name, "UserRoamingConfigStore"))
            {
                hasRoam = true;
                break;
            }
        if (hasRoam)
            outText = addSteamToRoam(tops, ids, eol);
        else
            outText = buildRoamRoot(tops, nullptr, ids, eol);
    }
    else
    {
        Seg *canonical = pickCanonicalSteam(tops, allSteams);

        // ---- Merge unique settings from stray Steam blocks into the canonical
        //      one so nothing present in any Steam block is ever lost. ----
        vector<Seg> mergedKids;
        for (Seg *stray : allSteams)
        {
            if (stray == canonical)
                continue;
            for (auto &k : stray->kids)
            {
                // Only import meaningful settings, not the bits we regenerate.
                if (ieq(k.name, "apps"))
                    continue;
                if (ieq(k.name, "CloudEnabled") && !k.isBlock)
                    continue;
                if (ieq(k.name, "Steam") && k.isBlock)
                    continue; // nested Steam: flattened separately (it's in allSteams)
                // Never move a container that holds the canonical Steam (e.g. a
                // UserRoamingConfigStore/Software/valve chain wrapped inside a
                // stray Steam). Moving it into the canonical's kids would make
                // the canonical a descendant of itself, and renderSeg would
                // then recurse forever on the cycle.
                if (containsSeg(k, canonical))
                    continue;
                if (!hasKid(*canonical, k.name))
                    mergedKids.push_back(std::move(k));
            }
        }

        // ---- Drop stray Steam blocks (their data was merged above). ----
        for (Seg *stray : allSteams)
            if (stray != canonical)
                stray->drop = true;

        // ---- Drop stray "apps" blocks not inside the canonical Steam. ----
        for (auto &top : tops)
            markStrayApps(top, canonical, false);

        for (auto &m : mergedKids)
            canonical->kids.push_back(std::move(m));

        // ---- Transform the canonical Steam: force the outer CloudEnabled to
        //      "0" and rebuild the per-game apps block. ----
        transformCanonical(*canonical, ids, eol);

        // ---- Prune container blocks (valve/Software) that became empty. ----
        for (auto &top : tops)
            markEmptyContainers(top);

        // ---- Render. If a UserRoamingConfigStore holds the canonical Steam,
        //      keep the existing structure verbatim; otherwise build one. ----
        bool canonicalUnderRoam = false;
        for (const auto &top : tops)
            if (top.isBlock && ieq(top.name, "UserRoamingConfigStore") && containsSeg(top, canonical))
            {
                canonicalUnderRoam = true;
                break;
            }
        // A Steam wrapped in ANOTHER store block (roam > local > Software >
        // valve > Steam) must not be rendered verbatim: that leaves a nested
        // store and no Steam reachable from the roam root. Rebuild instead.
        if (canonicalUnderRoam)
        {
            for (const auto &top : tops)
                if (top.isBlock && ieq(top.name, "UserRoamingConfigStore") &&
                    containsSeg(top, canonical) && storeWrapperOnChain(top, canonical))
                {
                    canonicalUnderRoam = false;
                    break;
                }
        }
        if (canonicalUnderRoam)
        {
            // ---- Fold stray top-level content into the canonical roam. ----
            Seg *canonicalRoam = nullptr;
            for (auto &top : tops)
                if (top.isBlock && ieq(top.name, "UserRoamingConfigStore") && containsSeg(top, canonical))
                {
                    canonicalRoam = &top;
                    break;
                }

            string steamExtra; // steam-level settings folded into Steam
            string rootExtra;  // root-level content folded into the roam
            // Steam-level settings must land at the canonical Steam's OWN child
            // indent, which is not always the standard 4 tabs (a nested roam or
            // a one-line input can put Steam at a shallower or deeper level).
            string steamChildIndent = indentOf(canonical->header) + "\t";
            for (auto &top : tops)
            {
                if (top.drop)
                    continue;
                if (&top == canonicalRoam)
                    continue; // rendered below
                if (ieq(top.name, "Software"))
                {
                    top.drop = true; // stray duplicate block; data was merged above
                    continue;
                }
                if (!top.isBlock && ieq(top.name, "CloudEnabled"))
                {
                    top.drop = true; // canonical block provides the global switch
                    continue;
                }
                if (isStrayBrace(top))
                {
                    top.drop = true; // stray "}" braces from truncated files
                    continue;
                }
                if (isStoreBlock(top))
                {
                    // A second store (duplicate roam or a UserLocalConfigStore):
                    // unwrap it so its content becomes root-level siblings of the
                    // canonical roam rather than a nested store block.
                    for (auto &k : top.kids)
                        if (!k.drop)
                            rootExtra += foldJunk(k, "\t", eol, false);
                    top.drop = true;
                    continue;
                }
                if (isSteamLevel(top))
                    steamExtra += foldJunk(top, steamChildIndent, eol, false);
                else if (top.isBlock || !top.name.empty())
                    rootExtra += foldJunk(top, "\t", eol, false);
                else
                    rootExtra += foldJunk(top, "\t", eol, true);
                top.drop = true;
            }

            if (!steamExtra.empty())
            {
                Seg raw;
                raw.header = steamExtra;
                canonical->kids.push_back(std::move(raw));
            }
            if (!rootExtra.empty())
            {
                Seg raw;
                raw.header = rootExtra;
                canonicalRoam->kids.push_back(std::move(raw));
            }

            for (const auto &top : tops)
            {
                if (top.drop)
                    continue;
                outText += renderSeg(top);
            }
        }
        else
        {
            outText = buildRoamRoot(tops, canonical, ids, eol);
        }
    }

    return writeOutput(sharedConfigPath, outText, ids.size());
}

bool CloudDisabler::iterateSharedConfig(const string &userDataPath, const string &acfIds)
{
    FileUtility fileUtility;
    for (const auto &entry : filesystem::directory_iterator(userDataPath))
    {
        if (entry.is_directory())
        {
            string steamID = entry.path().string();
            replace(steamID.begin(), steamID.end(), '\\', '/');

            string remotePath = steamID + "/7/remote";
            if (filesystem::exists(remotePath))
            {
                string sharedConfigPath = remotePath + "/sharedconfig.vdf";
                if (filesystem::exists(sharedConfigPath))
                {
                    string sharedConfigText = fileUtility.readFileContents(sharedConfigPath);
                    replaceAppsBlock(sharedConfigPath, sharedConfigText, acfIds);
                }
            }
        }
    }
    return true;
}

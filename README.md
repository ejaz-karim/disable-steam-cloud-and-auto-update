# Steam Cloud and Auto-Update Disable

This program allows you to manage the cloud synchronization and auto-update settings for games in your Steam library. By running this program, you can disable cloud storage and auto-updates for all the games you have downloaded on Steam. 

## Prerequisites

- Steam client installed on your computer

## Usage

Follow these steps to use the "steam-cloud-and-autoupdate-disable" program:

1. Open the Steam client on your computer.
2. In the Steam settings, enable cloud synchronization. This step is necessary to ensure that the changes made by the program take effect.
3. Close the Steam client to ensure that it is not running while the program is executing.
4. Run the "steam-cloud-and-autoupdate-disable" program.
5. The program will prompt you to enter the directories for the configuration files.
   - Enter the directory for the "sharedconfig.vdf" file, which contains the cloud synchronization and auto-update settings.
   - Enter the directory for the "libraryfolders.vdf" file, which lists all the game IDs.
6. The program will process the files and disable cloud storage and auto-updates for the games in your Steam library.
7. Once the program completes its execution, it will display a success message.
8. You can now launch Steam and verify that the cloud synchronization and auto-update settings have been disabled for the respective games.

**Note:** It is important to keep in mind that modifying configuration files can have unintended consequences. Use this program at your own risk, and make sure to backup your files before running it.

## About

The "steam-cloud-and-autoupdate-disable" program is a command-line tool written in C++. It utilizes the Steam configuration files to manage the cloud synchronization and auto-update settings for games. By disabling these features, you can have more control over the game data storage and prevent automatic updates for your games.

## Troubleshooting

If you encounter any issues or have questions regarding the "steam-cloud-and-autoupdate-disable" program, please refer to the following:

- **"apps" not found in /libraryfolders.vdf:** If you receive this message, it means that the program could not locate the "apps" section in the "libraryfolders.vdf" file. This can occur if you have recently installed Steam or if the file is missing or corrupted. Make sure the file exists and try running the program again.
- **"apps" not found in /sharedconfig.vdf:** This message indicates that the program could not find the "apps" section in the "sharedconfig.vdf" file. This can happen if the file is missing or if the cloud synchronization and auto-update settings have not been enabled in Steam. Ensure that the file is present and try running the program again. 

**Note:** Also, this could mean that you have not downloaded any games on steam as you might have a fresh install.

If you encounter any other issues or need further assistance, please seek help from the program's developer or refer to the program's documentation.
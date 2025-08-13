# TestMiiInternet

TestMiiInternet is a Homebrew Application for the Nintendo Wii that allows users to test their internet connection speed directly from their console.
## Features

- **Ping Testing**: Test latency to Google.com.
- **Download Speed Testing**: Measure download speeds from GitHub.
- **Upload Speed Testing**: Measure upload speeds to File.io.
- **Comprehensive Summary**: One-line summary of all test results.

## Installation

1. Download the latest release from the GitHub releases page.
2. Extract the "Apps" folder from the provided Zip file (`TestMiiInternet.zip`).
3. Place the extracted folder on the root of your SD Card or USB Drive.
4. Launch through the Homebrew Channel.

## Build Requirements

### Prerequisites
- **DevKitPro** - Nintendo Wii development environment.
- **LibOGC** - Official GameCube/Wii library.
- **cURL-WII** - HTTP/HTTPS networking library for Wii.


Please ensure all dependencies are properly installed before building.

### Building
```bash
make
```

Use the `make` command to compile the application.

## Credits

- **Made by**: Masaru Mamoru
- **Peer Reviewed by**: Abdelali221
- **Year**: 2025

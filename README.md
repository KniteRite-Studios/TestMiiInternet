# TestMiiInternet

TestMiiInternet is a (WIP) Homebrew Application for the Nintendo Wii that allows users to test their internet connection speed directly from their console.
## Features

- **Ping Testing**: Test latency to google.com
- **Download Speed Testing**: Measure download speeds from dropbox.
- **Upload Speed Testing**: Measure upload speeds to file.io.
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

## License
MIT License

Copyright (c) 2025 KniteRite Studios (Masaru Mamoru)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

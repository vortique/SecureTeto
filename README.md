# SecureTeto
> Kasane Teto's encrypted file storage solution for storing her precious baguette images.

**SecureTeto** is a fast, secure, and user-friendly encrypted file storage program written with C (for backend, archiving etc.) and Python (for GUI, PyQt6).

It allows users to create encrypted containers (vaults) that can be accessed securely from Python GUI, providing seamless access to encrypted files.

## ⚠️ Development Status

**SecureTeto** is in **early development**. Currently working on:
- GUI design and implementation
- Adding more features to the C backend for vault management, enc/dec and binding to Python.

Not ready for production use yet!

## Early screenshots of the Python GUI (subject to change):
![SecureTeto GUI](assets/screenshots/current.png)

## Roadmap

- [x] Design unique archive format for encrypted vaults
- [x] Implement archive creation and extraction
- [x] Command-line interface (CLI) for vault management
- [ ] Develop more Python bindings for C backend
- [ ] Implement GUI features for vault creation, file management, and encryption/decryption

### In future releases...

- [ ] Cross-platform support (Windows, macOS, Linux)
- [ ] Mountable encrypted vaults as virtual drives

## Installation

### On Windows
---

#### Pre-requisites

##### MSYS2

1. Download and install [MSYS](https://mingw-w64.org/doku.php/download/mingw-builds)
2. Search **MSYS2 MINGW64** in the search bar and open
3. Write `pacman -Syu` in the terminal
4. If it closes the terminal or says to close it, close it and open it again
5. Write `pacman -Su` in the terminal
6. Install build deps with `pacman -S mingw-w64-x86_64-toolchain` (select all for installation)
7. Install libsodium with `pacman -S mingw-w64-x86_64-libsodium`
8. Install git with `pacman -S mingw-w64-x86_64-git` (Optional if you want use MSYS all time)

##### Python

1. Download and install [Python 3.11.9](https://www.python.org/downloads/release/python-3119/) (Add to PATH)

#### Install SecureTeto

1. Clone the repository and navigate to the root directory:
```bash
git clone https://github.com/vortique/SecureTeto.git
cd SecureTeto
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

1. Open MSYS2 MinGW64 terminal to run `make`:
```bash
make
```

1. Run the program:
```bash
.\bin\secu_engine.exe
```

**NOTE**: GUI is not available yet.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Steal the code (responsibly) and use it in your projects or print it with printer and burn them (don't do that), but please give credit to the original author. And don't steal Teto's baguette, that's her!

# IDR
Interactive Delphi Reconstructor
IDR (Interactive Delphi Reconstructor) – a decompiler of executable files (EXE) and dynamic libraries (DLL), written in Delphi 
and executed in Windows32 environment.

The program firstly is intended for the companies, engaged by development of anti-virus software. It can also help programmers 
to recover lost source code of programs appreciably.

The current version of the program can process files (GUI and console applications), compiled by Delphi compilers of versions 
Delphi2 – Delphi XE4.

Final project goal is development of the program capable to restore the most part of initial Delphi source codes from the 
compiled file but IDR, as well as others Delphi decompilers, cannot do it yet. Nevertheless, IDR is in a status considerably 
to facilitate such process. In comparison with other well known Delphi decompilers the result of IDR analysis has the greatest 
completeness and reliability. Moreover interactivity does work with the program comfortable and (we shall not be afraid of 
this word) pleasant.

IDR make static analysis (analyzed file is not loaded to memory and executed) that allows to safely investigate viruses, 
trojans and other malware applications, those which executing is dangerous or is not desirable.

The program does not require any installation activity and does not create any records in Windows registry.

Use Borland C++ Builder 6 to build this project.

IDR dont require any installations, just copy idr.exe, dis.dll, icons.dll, idr.ico and *.bin files
to IDR home directory. Message "Cannot Initialize Disasm" means that file dis.dll is absent.

!!!
Knowledge bases for various Delphi versions can be found here (find files kb*.7z and syskb*.bin).

TntControls can be found at https://github.com/rofl0r/TntUnicode

!!!
In project options use build Release version with no Optimization.

## Modern CLI (community)

A cross-platform, headless command-line port of IDR — for automation, CI, and AI agents (no VCL / no GUI):

| | |
|---|---|
| **Repository** | https://github.com/2003magic/idr-cli |
| **Build** | CMake + C++17; Windows / macOS / Linux |
| **Commands** | `analyze` (JSON on stdout), `info`, `export` (lst / idc / pas / idp) |
| **License** | MIT (same as IDR); uses the same `syskb*.bin` knowledge bases |

This repository remains the reference for the interactive GUI. Script and headless users may prefer **idr-cli**.

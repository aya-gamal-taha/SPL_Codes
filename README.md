# 📌 SPL_Codes  
**System Programming - Linux**  

This repository contains simple C implementations of common Unix utilities.  
Each program is written to mimic the basic behavior of its Unix counterpart,  
following standard Unix specifications without extra flags or options.

---

## 📂 Implemented Utilities

| Utility | Description |
|---------|-------------|
| **pwd** | Prints the current working directory. |
| **echo** | Prints the given arguments to standard output. |
| **cp** | Copies a file from a given source to a given destination. |
| **mv** | Moves (or renames) a file from source to destination. |

---

## 🛠 Files in the Repository

- `pwd.c` — Implementation of the `pwd` command.
- `echo.c` — Implementation of the `echo` command.
- `cp.c` — Implementation of the `cp` command.
- `mv.c` — Implementation of the `mv` command.

---

## ⚙ Compilation Instructions

Compile each utility separately using `gcc`:

```bash
gcc pwd.c -o pwd
gcc echo.c -o echo
gcc cp.c -o cp
gcc mv.c -o mv


## ▶ Usage Examples
./pwd
# Output: /home/user

./echo Hello World
# Output: Hello World

./cp file.txt /tmp/file_copy.txt
# Copies file.txt to /tmp/file_copy.txt

./mv /tmp/file.txt /tmp/new_name.txt
# Moves/renames file.txt to new_name.txt

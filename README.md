# Evanesque

A minimalistic stack-based toy language with a twist: it forgets random keywords during execution each time you define a new one.

Every time you define a new word with `: ... ;`, an existing word will "vanish", including the one you just wrote. Long-lived programs inevitably
collapse when the interpreter forgets the very words they depend on.

This obviously isn't practical at all and it a fun one-night project :)

# Installation and setup
```sh
git clone https://github.com/brayevalerien/evanesque
cd evanesque
make
sudo make install # optional, global install
./evanesque < examples/hellow_world.evq 
```

# Features
- data stack (cell = long)
- user-defined words via `: name ... ;`
- "vanish": each new definition erases a random existing word... the language forgets itself over time!
- built-ins: `+` `-` `*` `/` `=` `<` `>` `.` `emit` `key` `dup` `drop` `swap` `over`
- control flow inside definitions: `begin` `while` `repeat`
- `tuck`
- C-style block comments: `/* ... */`
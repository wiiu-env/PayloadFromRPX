# Standalone payload.elf loader
This is a payload that should be run with [MochaLite](https://github.com/wiiu-env/MochaLite) before the System Menu.
It's exploits the Cafe OS and maps 8 MiB of usable memory from 0x30000000...0x30800000 (physical address) to 0x00800000... 0x01000000 (virtual address) where a payload will be loaded. You may need to hook into the kernel and patch out some thing to gain persistent access to this area.
The loaded `hook_payload.elf` needs to be mapped to this memory area.

## Usage
Put the `payload.elf` in the `sd:/wiiu/` folder of your sd card and start the application.
If no `payload.elf` was found on the sd card, a IOSU exploit will be executed which forces the `default title id` to the Wii U Menu (in case of `system.xml` changes)

## Building
Make you to have [wut](https://github.com/devkitPro/wut/) installed and use the following command for build:

```
make
```

If you change any IOSU related changes, you need to do a `make clean` before compiling

## Credits
- orboditilt
- Maschell
- many many more
Parts taken from: 
https://github.com/FIX94/haxchi
https://github.com/dimok789/mocha
https://github.com/dimok789/homebrew_launcher
https://github.com/wiiudev/libwiiu/blob/master/kernel/gx2sploit/
[...]
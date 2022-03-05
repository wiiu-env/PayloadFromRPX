# Standalone payload.elf loader
This is .rpx is meant to run in a envrionment with sd and codegen (jit) access.
It's exploits the Cafe OS and maps 8 MiB of usable memory from 0x30000000...0x30800000 (physical address) to 0x00800000... 0x01000000 (virtual address) where a payload will be loaded. You may need to hook into the kernel and patch out some thing to gain persistent access to this area.
The loaded `payload.elf` needs to be mapped to this memory area.

## Usage
Put the `payload.elf` in the `sd:/wiiu/` folder of your sd card and start the application.
If no `payload.elf` was found on the sd card, a IOSU exploit will be executed which forces the `default title id` to the Wii U Menu (in case of `system.xml` changes)

Special button combinations:
- Hold R while launching to skip `payload.elf` launching.

## Reset the default title id
When loading this payload in a coldboot environment a payload.elf may want to force the default title id to the Wii U Menu. This loader offers some callbacks to the `payload.elf` to achieve such behaviour.
The `payload.elf` will be loaded with some special arguments. As normal, the first argument is the name of current running RPX, but afterwards a list of callbacks is provided.

Example implementation of the loader:
```
argc = 3;
argv[0] = "safe.rpx";                                       // original argument
argv[1] = "void forceDefaultTitleIDToWiiUMenu(void)";       // signature of the first callback function
argv[2] = &forceDefaultTitleIDToWiiUMenu;                   // pointer to first callback function.
int res = ((int (*)(int, char **)) entryPoint)(argc, arr);  // call the payload.elf with some special arguments.
```

Inside the payload.elf you may want to do something like this:
```
for (int i = 0; i < argc; i++) {
            if(strcmp(argv[i], "void forceDefaultTitleIDToWiiUMenu(void)") == 0){
                if((i + 1) < argc){
                    i++;
                    void (*forceDefaultTitleIDToWiiUMenu)(void) = (void (*)(void)) argv[i];
                    forceDefaultTitleIDToWiiUMenu();
                }
            }
        }
```

Currently the following callbacks are provided: 
```
void forceDefaultTitleIDToWiiUMenu(void) = Reverts the coldboot into a specific title by forcing it the Wii U Menu. Caution: This will perform a IOSU exploit.
```

## Building
Make you to have [wut](https://github.com/devkitPro/wut/) installed and use the following command for build:

```
make
```

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t payloadfromrpx-builder

# make 
docker run -it --rm -v ${PWD}:/project payloadfromrpx-builder make

# make clean
docker run -it --rm -v ${PWD}:/project payloadfromrpx-builder make clean
```


## Format the code via docker

`docker run --rm -v ${PWD}:/src wiiuenv/clang-format:13.0.0-2 -r ./source -i`

## Credits
- orboditilt
- Maschell
- many many more

Parts taken from: 
- https://github.com/FIX94/haxchi  
- https://github.com/dimok789/mocha  
- https://github.com/dimok789/homebrew_launcher  
- https://github.com/wiiudev/libwiiu/blob/master/kernel/gx2sploit/  
[...]
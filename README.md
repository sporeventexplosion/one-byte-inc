# 1-Byte INC in 64-bit code!

![Logo](/logo.png)

Are you still mad at AMD for destroying the one-byte encoding of INC when they stomped over the whole row from 40h to 4Fh with their newfangled "REX" prefixes?
Think of the code density!
Be mad no more with `one_byte_inc`!!!

As we all know, x86-64 CPUs all support *compatibility mode*, so the instruction decoder still needs to support the 1-byte INC.
If only we can access it in 64-bit code...

Well, now you can!!!

## Usage

Simply include `one_byte_inc.h` in your source and link with `-lone_byte_inc`.

In your code, initialize the library using `init_one_byte_inc()`, which returns 0 on success.
-1 is returned on error and errno is set.

After that, just use `one_byte_inc()` for all your 1-byte incrementing needs!

## Example

```c
#include <stdio.h>
#include "one_byte_inc.h"

int main() {
    init_one_byte_inc();
    printf("%d\n", one_byte_inc(89017)); // prints: 89018
}
```

See also the example in `example.c`.

## Building

Run `./build.sh` to generate static and dynamic libraries, as well as the example binary.

## How does it work?

umm... uhh...

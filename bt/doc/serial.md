# Serial protocol

The nrf52832 and RP2040 are connected by only a single UART and open-drain interrupt line. Both systems can
wake each other up, so in reality the `BT_INT` line is more of an "attention" line, and is used to request the _other_
system send an "awake" message. In this way, we can have both systems arbitrarily sleep their processors (so long as the
interrupt line is set to wake them up).

## Framing

The underlying protocol is designed to allow lossy data transfer as well as proper lossless acked messages, so the transport is split
between framing and messages.

Each transmission on the UART is quantized into 16-byte "chunks". A single frame is made up of at most 16 chunks (i.e. each frame is at most 256 bytes). The
first of the chunks always begins with:

```
B->M | 0x4d | 0x77 | 0bCCCC ' LLLL | 0bcccccccc |
B<-M | 0x6d | 0x77 |              ''            |
          <sync>          <length / checksum>
            |                      |
            |                      \--------- little-endian 16bit word with low 4 bits indicating number of remaining chunks
                                              (excluding this one) and remaining bits are 12-bit CRC checksum
            \----- must be present, sync word. first byte is "uppercase" 
                   in transmissions from the RP2040 bytes chosen to read 'Mw'/'mw' in a debugger
```

Frames are considered the smallest "atomic" chunk of data -- upper layers of the protocol have no meaning for a "partially received frame". However, 
each constituent chunk of a frame is 

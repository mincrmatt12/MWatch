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
            |                                 (excluding this one) and remaining bits are 12-bit CRC checksum (i.e. checksum
            |                                 is 0bCCCCcccccccc length is 0bLLLL)
            \----- must be present, sync word. first byte is "uppercase" 
                   in transmissions from the RP2040 bytes chosen to read 'Mw'/'mw' in a debugger
```

Frames are considered the smallest "atomic" chunk of data -- upper layers of the protocol have no meaning for a "partially received frame". The system is designed
so that data is received chunkwise, with timeouts for partially transmitted chunks.

## Pipes

Logically, the protocol manages a series of data pipes (sort of like USB endpoints, although with less protocol-level semantics). There are three types of pipes: windowed, unbuffered and control.
corresponding roughly to TCP/UDP-like data transmission and USB control-transfers respectively.

Each pipe has an 8-bit ID, of which the pipe settings are entirely transparent to the protocol (specifically, each system knows
about the settings for each pipe ID out-of-band -- for dynamically allocated IDs this might happen in-band, but will occur as commands)

Control pipes work in variable-sized messages that are each acked/naked atomically. As a result, any message must fit within a 252-byte frame's data, although in practice this is further limited to 64 bytes.
Each message is sent and processed in order, returning a result message (or a NAK). Commands are also idempotent via a frame counter.

Windowed pipes could be implemented on top of control pipes, however are handled at a lower level for efficiency. They implement a TCP-style data stream with ACKing & in-order guarantees.

Unbufferd pipes are the simplest form of pipe: they are effectively just raw data writes with only the framing checksum to authenticate them. They are never retransmitted and come in arbitrary
sized chunks.

## Packets

Each frame contains some stream of packets. These packets can be though of as a sort virtual machine interacting with the core protocol. Commands include things like "send data
to pipe" or "update window state for pipe".

Packets each start with a 1-byte packet ID. The high bit of the packet ID determines whether this packet is "ack-controlled", i.e. that it requires a frame ID to be assigned. Each
frame can be divided into the packets that are ack-controlled and not ack-controlled. The ack-controlled part of a frame is always assigned a unique ID that may be retransmitted if the underlying
frame is corrupted. The first ack-controlled packet in a frame is called the ack-header packet. The ack-header packet's data is always prefixed with the 16-bit frame ID.



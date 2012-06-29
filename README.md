megas2
======


Objectives
----------

`megas2` is an ATMEGA32 full system emulator designed primarily with simplicity
and hackability in mind. It is intended to help me with my various ATMEGA
projects whenever they:

* use sophisticated or ad-hoc peripherals that are not available in mainstream
  simulation packages (e.g. Ethernet chips, or sensors connected to a complex
  process)
* have such complex code that it requires numerous testing cycles that would be
  time-consuming and wear out the Flash memory of the chip

It cannot be stressed enough that `megas2` is intentionally designed to favor
simplicity over (often unnecessary) accuracy. As such, it is not cycle-exact,
communication over SPI/I2C is instantaneous, etc. (a more complete list of
limitations can be found later in this document). The emulation is only as
accurate as is stricly needed for moderately complex projects such as my own.

Performance is not a big priority either. Even though simulating a machine
that is literally 100 times slower than the host is not an excuse to be
careless, I would rather avoid premature optimization in this case. Should the
emulation turn out to be too slow (which I doubt), only then will I consider a
rethinking and re-engineering process to be in order.

Last but not least, `megas2` is also a nice exercise for stretching those
programming muscles in a new direction. It is not my first emulator writing
experience, and I know it can be quite fun and ultimately educational.


Related Projects
----------------

`megas2` is a personal project, and, though I would certainly be glad for it
to be of use to anyone else, I would certainly not recommend it before trying
these two alternatives first:

* [simavr](http://gitorious.org/simavr) is the newest kid on the block and it
  seems to be developing into a highly professional and reliable solution for
  your ATMEGA/AVR simulation needs. Unfortunately, it lack support for the very
  ATMEGA variants I am interested in.
* [simulavr](http://savannah.nongnu.org/projects/simulavr) seems fairly
  complete, accurate, and hackable. That said, I wasn't able to get it to work
  properly on my system, and I wasn't satisfied with the interfaces it provides
  for you to attach your own simulated devices.

Both of these solutions are also far, far more complex than is generally needed
for simple projects, and this can be an impediment to the hackability that is
inevitably required in these situations.


Limitations
-----------

The following is an inventory of the emulation limitations in `megas2`.

### ATMEGA32 emulation

#### CPU core

* Emulation is not cycle-exact (all instructions take 1 cycle)
* Interrupts are not supported
* `FMUL*` instructions are not supported
* `SPM` instructions are not supported
* `SLEEP`, `BREAK`, `WDR` instructions are not supported (they are treated as `NOP`)

#### Timers

* Not supported

#### ADC

* Not supported

#### SPI

* Only master transmitter/receiver behavior supported
* Operations complete instantaneously and always succeed
* SPI bandwidth settings are ignored

#### TWI

* Only master behavior supported
* Operations complete instantaneously and always succeed
* TWI bandwidth settings are ignored

#### USART

* Not supported

### SD Card emulation

* Only these commands are supported:
    * `GO_IDLE_STATE`
    * `SET_BLOCKLEN`
    * `READ_SINGLE_BLOCK`
    * `WRITE_SINGLE_BLOCK`
* Operations complete instantaneously
* Only 512-byte blocks are supported

### DS1307 (Real-Time Clock)

* Stub available, development in progress

### ENC28J60 (Ethernet controller)

* Stub available, development in progress

### Interconnection

* Tri-state and directionality not explicitly supported
* Analog pins not supported


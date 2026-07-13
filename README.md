# ESP32 RC Airboat

A remote-controlled airboat built in one week, almost entirely from scraps
around the house. It's driven by two above-water fans, steered from an Xbox
controller over Bluetooth, and runs on an ESP32 with a motor driver salvaged
out of a disassembled RC car. The hull is a plastic food container.

![The airboat during an on-water test](images/airboat.jpg)

*(Pictured mid-test in a bathtub — it's watertight and drives under its own power.)*

---

## Motivation

I built this for the sake of building it. No client, no assignment — I gave
myself a one-week deadline and a rule that it should come together from
things I already had lying around. The motor driver came out of an RC car I
took apart. The hull is a plastic food container. The frame is chopsticks and
popsicle sticks. Beyond just making something, it was a deliberate learning
exercise: I wanted to work through a full embedded project end to end —
wireless control, motor driving, power, and a physical build that actually
has to survive contact with water.

## How it works

The boat uses **differential thrust** to steer, like a tank. Two fans sit
above the waterline; spinning one faster than the other turns the boat, so
there's no rudder and nothing spinning underwater to seal against.

- An **ESP32** receives input from an Xbox controller over Bluetooth (using
  the Bluepad32 stack).
- Each control stick maps to one fan motor.
- The ESP32 sends PWM to an **MX1515 dual H-bridge**, which drives the two
  motors forward or reverse at any speed.
- Push both sticks forward to go straight, one forward and one back to spin
  in place, or different amounts to arc a turn.

## What it's made of

Almost everything was salvaged or repurposed:

- **Motor driver:** an MX1515 dual H-bridge, desoldered from a disassembled
  toy RC car.
- **Brain:** an ESP32 dev board.
- **Motors:** two 1.5–3V DC motors from an Arduino starter kit. I sanded a
  flat onto each motor body for more gluing surface, superglued them to the
  mounts, then zip-tied them down as backup so vibration couldn't shake them
  loose.
- **Props:** fan blades from the same kit.
- **Hull:** a plastic food container, reinforced with chopsticks along the
  top rim and popsicle sticks on the sides that double as the motor mounts.
- **Power:** 4×AA for the motors; the ESP32 is currently tethered over USB.

---

## The challenges (the honest version)

Almost none of the week was spent on the parts that sounded hard. Bluetooth
pairing, reading the sticks, and PWM came together quickly. The entire
project nearly died on **power** — and the story is worth telling straight,
because the dead ends taught me more than the parts that worked first try.

### It worked, then a motor wouldn't stop

The first working version ran the ESP32 and the motors from the same battery.
It would work for a moment, then the controller would disconnect and a motor
would spin nonstop — and the *only* thing that fixed it was pulling power
entirely. Every restart, same cycle: fine at idle, chaos the instant I pushed
a stick.

### Reading the serial output: brownouts

The serial monitor was full of garbled characters and a repeating boot
banner. Eventually one clear line explained everything: **"Brownout detector
was triggered."** The ESP32 wasn't just glitching — it was resetting. The
motors' current surge was sagging the battery below the voltage the ESP32
needs to survive, so the chip browned out and rebooted mid-command. Because
it reset during the fault, it never ran the code that was supposed to stop
the motor — so the motor latched on. The garbled text was the serial line
collapsing as the voltage dropped.

### The salvaged chip fought back

Fixing the obvious brownout didn't end it. The MX1515 out of the RC car had
its own problems: it would latch a motor on and ignore my inputs completely,
recoverable only by cutting power. Then, after a change, the behavior shifted
— the motor would run for a few seconds and shut itself off, then do it again
on the next stick touch. That pattern is a chip overheating and
thermal-protecting itself, cooling, and re-arming. Something was pulling far
too much current every time that channel switched on.

### Digging into the datasheet

The MX1515's datasheet is a terse, Chinese-language document for a toy-grade
part. Buried in the application notes was the answer to the lock-up: the
chip's logic supply **requires at least 4.7µF of capacitance to ground**, or
it can latch into a locked state after an overload and refuse inputs until
power is removed. My design had zero capacitance there. Worse, that was a
mistake I'd built in myself — to keep all my wiring on one side of the board,
I'd powered the driver's logic pins directly from a GPIO, and I'd checked the
current math but missed the capacitor requirement entirely. Adding a 10µF cap
to that node killed the lock-up exactly as the datasheet promised.

### The multimeter and the real fix

With the lock-up gone, the overheating remained, so I pulled the chip and
went through it with a multimeter — verifying the pinout matched the
datasheet, checking for shorted internal transistors, comparing the two
channels against each other. And the deeper realization finally landed: the
ESP32 and the motors have **opposite needs**. Motors pull big, spiky current
and don't care if the voltage droops. The ESP32 sips current but needs a
rock-steady supply. Forcing both onto one marginal battery served neither.

The fix was to **split the supplies**: motors on the battery, ESP32 on its
own regulated USB power, joined only at a shared ground. The moment I did,
the brownouts stopped. The ESP32 stayed powered through every current spike,
which meant it actually ran its own safety code — so when the controller
disconnects now, the motors stop cleanly instead of running away. My control
code had been correct the whole time; it just never had a processor that
stayed alive long enough to run it.

---

## Outcome

It works. It's watertight, it drives under its own power, and it steers by
differential thrust the way it's supposed to. The photo above is from an
on-water test.

And it came together inside the one-week deadline, built almost entirely from
scraps — a food container, kitchen-drawer sticks, kit motors, and a driver
chip that started its life in a toy car. The parts that made me proud weren't
the ones that went smoothly; they were the power problems I had to actually
understand — reading brownout logs, decoding a bad datasheet, and reasoning
about why two loads can't always share one supply — instead of swapping parts
until something worked.

### What's next

- **Cut the cord.** The ESP32 is still USB-tethered for power. The obvious
  next step is putting it on its own battery so the boat is fully wireless.
- **A failsafe for range.** Bluetooth can take a while to register that a
  controller has gone out of range. A heartbeat timeout that cuts thrust when
  fresh input stops arriving would make it safer on open water.
- **Real thrust.** Kit motors with fan blades produce modest thrust — fine on
  calm water, weak against wind. Proper propellers and faster motors are the
  upgrade path.
- **Off the breadboard.** A custom PCB would replace the breadboard-and-jumper
  rat's nest currently riding in the hull.

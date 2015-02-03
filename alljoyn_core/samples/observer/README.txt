# Sample: toy home security system simulation

## Introduction
This sample is a simulation of a rudimentary home security system. Our
hypothetical security system monitors all doors in your house, and lets you
know in real time whether they're open or closed, and who passes through a
door. In addition, it allows for the remote opening an closing of doors.

The sample is split over two applications: `door_provider` is the security
system itself that publishes the state of the doors, `door_consumer` is a
simple monitoring user interface through which the user can monitor the state
of the doors, and can remotely open or close doors.

Note: these sample applications are equivalent to, and compatible with, the
door_provider and door_consumer samples for the Data-driven API.

## Usage

As the samples are built with the builtin router, you do not need to have the daemon running.

### Start one or more `door_provider`s

    $ door_provider <location1> [<location2> [... [<locationN>] ...]]

e.g.:

    $ door_provider home office

`door_provider` will simulate as many doors as you supply locations on the
command line. You need to supply at least one location.

You will be dropped into a primitive command line user interface where you can
issue simulation commands. To keep the interface simple, the application
continuously cycles through all doors it maintains, and you cannot choose which
door will be the subject of your next simulation command.

The following commands are supported:

    q         quit
    f         flip (toggle) the open state of the door
    p <who>   signal that <who> passed through the door
    r         remove or reattach the door to the bus
    n         move to next door in the list
    h         show this help message

### Start a `door_consumer`

    $ door_consumer

The application will monitor the state of all doors that are published on the
bus, and will print notifications whenever doors appear, disappear, or change
state. In addition, you can perform the following actions:

    q             quit
    l             list all discovered doors
    o <location>  open door at <location>
    c <location>  close door at <location>
    k <location>  knock-and-run at <location> (will cause a closed door to
                  briefly open)
    h             display this help message

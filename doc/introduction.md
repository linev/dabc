# DABC introduction {#dabc_introduction}

The **Introduction** shortly describes the aspects of the Data Acquisition Backbone Core
framework to get an idea about its core components.

## Ideas

* Multithreading
* Zero-copy transport
* Many-nodes communications
* Flexible configurations
* Easy control interfaces


## Role and functionality of the objects

### Modules
All processing code runs in module objects.
There are two general types of modules: the dabc::ModuleSync and the dabc::ModuleAsync.

#### Class dabc::ModuleAsync
Several asynchronous modules may be run by the *same thread*.
The thread processes an  *event queue* and executes
appropriate  *callback functions*
of the module that is the receiver of the event. Events are fired for data input
or output, command execution, and if a requested resource (e.g. memory buffer)
is available. **The callback functions must never block the working thread**.
Instead, the callback must *return* if further processing requires
to wait for a requested resource. Thus each callback function must check the
available resources explicitly whenever it is entered.


#### Class dabc::ModuleSync
Each synchronous module is executed by a dedicated thread.
The thread executes a method [dabc::ModuleSync::MainLoop()](\ref dabc::ModuleSync::MainLoop)
with arbitrary code, which **may block**
the thread. In blocking calls of the framework (resource or
port wait), optionally command callbacks may be executed
implicitly ("non strictly blocking mode"). In the "strictly
blocking mode", the blocking calls do nothing but wait.
A *timeout* may be set for all blocking calls; this can
optionally throw an exception when the time is up. On timeout
with exception, either the [MainLoop()](\ref dabc::ModuleSync::MainLoop) is left and the exception
is then handled in the framework thread; or the [MainLoop()](\ref dabc::ModuleSync::MainLoop) itself
catches and handles the exception. .


### Commands
A module may process dabc::Command object in dabc::Module::ExecuteCommand() method.
If necessary, module can declare commands definition, that control system could
know that kind of commands can be submitted to the module.


### Parameters
A module may register dabc::Parameter objects.
Parameters are accessible by name; their values can be monitored and optionally changed by
the controls system. Initial parameter values can be set from xml configuration files.


### Manager
The modules are organized and controlled by one manager object of
dabc::Manager; this singleton instance is persistent independent of the application's state.
One can always access the manager via dabc::mgr variable.

The manager is an *object manager* that owns and keeps all
registered basic objects into a folder structure.

Manager dispatches different events in the objects and deliver them to control system
(if such configured). This covers registering, sending, and receiving of commands; registering,
updating, unregistering of parameters; error logging and global error handling.

The manager receives and *dispatches commands*
to the destination modules where they are queued and eventually executed
by the modules threads.
The manager has an independent manager thread, used for
manager commands execution, parameters timeout processing and so on.


### Memory and buffers
Data in memory is referred by dabc::Buffer objects.
Allocated memory areas are kept in dabc::MemoryPool objects.

In general case dabc::Buffer contains a list of references to scattered memory
fragments from memory pool. Typically a buffer references exactly one segment.
Buffer may have an empty list of references.

The auxiliary class dabc::Pointer offers methods to transparently
treat the scattered fragments from the user point of view
(concept of "virtual contiguous buffer").
Moreover, the user may also get direct access to each of the fragments.

The buffers are provided by one or several memory pools
which preallocate reasonable memory from the operating system.
A modules communicates with a memory pool
via a [pool handles](\ref dabc::PoolHandle) .

A new buffer may be requested from a memory pool by size.
Depending on the module type and mode, this request may either block until an
appropriate buffer is available, or it may return an error value
if it can not be fulfilled. The delivered buffer has at
least the requested size, but may be larger. A buffer as
delivered by the memory pool is contiguos.

Several buffers may refer to the same fragment of memory.
Therefore, the memory as owned by the memory pool has a
reference counter which is incremented for each buffer
that refers to any of the contained fragments. When a user frees
a buffer object, the reference counters of the referred
memory blocks are decremented. If a reference counter becomes
zero, the memory is marked as "free" in the memory pool.


### Ports
Buffers are entering and leaving a module through ports.
There are [input](\ref dabc::InputPort) and [output](\ref dabc::OutputPort) ports.
Each port has a buffer queue of configurable length.
A module may have several input and (or) output ports.
The ports are owned by the module.

Depending on the module type, there are different possibilities to
work with the ports in the processing functions of the module.
These are described in respective sectiops of dabc::ModuleSync and dabc::ModuleAsync.

One could specify "blocking" parameter for output or/and input port to define
how queue between two ports behaves if it full. Possible values are:
* "connected"    - queue only blocks when both ports are connected (default)
* "disconnected" - queue only blocks when input port not connected
* "never"        - queue never blocks, buffers can be lost
* "always"       - queue is always blocks

One also could configure that happens when port connection is closed due to error.
It is done via "onerror" property in xml file or \ref dabc::Port::ConfigureOnError() method.
Allowed values are: "none", "close", "stop", "exit", "abort"



### Transport
Outside the modules the ports are connected to dabc::Transport objects.
On each node, a transport may either transfer buffers between
the ports of different modules (local data transport),
or it may connect the module port to a data
source or sink (e.~g.~ file i/o, network connection, hardware readout).

In the latter case, it is also possible  to connect ports of two modules on
different nodes by means of a transport instance of the same kind on
each node (e.~g.~ *InfiniBand verbs* transport connecting a sender module on node A
with a receiver module on node B via a *verbs* device connection).


### Device
In some cases devices managing creation of transport objects.
There are two examples: dabc::SocketDevice and verbs::Device.
These objects used to establish network connections between the nodes.

A device is persistent independent of the connection state
of the transport. In contrast, a transport is destroyed after connection is closed.


### Application
The dabc::Application class is a singleton object that represents the
running application of the DABC node (i.~e.~ one per system process).
It could provides the main configuration parameters and defines the runtime actions
in the different control system states.

User-specific application class is necessary when complex interaction between
user modules is required. But in many practical cases user just need to create
module sub-classes.


### Factory
The dabc::Factory class used to create user-specific classes like modules or
transports.


## Configuration and run

### XML file syntax

[XML files](\ref dabc_xmlfile_format) are used to configure any DABC application.


### Running application

Via **dabc_exe** or **dabc_run**


### Configuration parameters




## Control

### Web interface

### go4 gui

### Command shell



## History - first requirements

This was [first requirements draftt](@ref dabc_first_concept),
which was formulated in the beginning of 2007,
when DABC development was started. Not everything was implemented according to these ideas,
but main concept of data-flow engine was done

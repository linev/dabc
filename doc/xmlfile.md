# XML files format {#dabc_xmlfile_format}

## Introduction

XML file in DABC provide way to configure most components of user application.
This include generic application parameters, memory pool configuration, transport creation,
modules parameters and so on.


## Simple example

Lets consider simple Example.xml file.

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Example">
    <Run>
      <lib value="libDabcMbs.so"/>
    </Run>
    <MemoryPool name="Pool">
       <BufferSize value="65536"/>
       <NumBuffers value="100"/>
    </MemoryPool>
    <Module name="Repeat" class="dabc::RepeaterModule">
       <InputPort name="Input0" url="lmd://Generator"/>
       <OutputPort name="Output0" url="mbs://Stream"/>
    </Module>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

This is example of configuration file, were repeater module translate generated mbs events
into mbs stream-server. To start example:

    [shell] dabc_exe Example.xml



## XML file structure

Any dabc xml file should have top-level `<dabc>` node like:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
...
</dabc>
~~~~~~~~~~~~~~~~~~~~~

Inside dabc node one or several `<Context>` nodes can be placed.
Each context corresponds to separate process, started via `dabc_exe` executable.
For the context node following attributes can be set:

   - "name"  - name of the context
   - "host"  - (optional) host where process should run
   - "port"  - (optional) socket port number for communiction with process

Example:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Remote" host="lx-pool.gsi.de" port="3425">
...
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~


Inside context following nodes can be presented:

   - `<Run>`  different run parameters of the process
   - `<Application>` application configuration parameters
   - `<MemoryPool>` memory pool config
   - `<Module>` module
   - `<Connection>` connection

### Run parameters

In `<Run>` section different global parameters defined, which are normally set
in the beginning, when dabc_exe process is started. Here is table of parameters,
which can be specified in `<Run>` section.

| Name       | Description |
| --------:  | :---------- |
| lib        | Library to be loaded. Can appear several times |
| debuglevel | level of debug output |
| loglevel   | level of debug output to the file |
| logfile    | name of log file |
| sysloglevel  | level of output to the syslog |
| syslog     | prefix for the syslog, default "DABC" |
| runtime    | maximum execution time in seconds |
| halttime   | time required to halt application (default 0.7 s) |
| func       | Name of C funtion to be called to create modules.  |
| ssh_user   | User name used to login on host. Login without paasword should be possible |
| ssh_port   | ssh port number fopr login |
| ssh_timeout | timeout for ssh in seconds |
| ssh_test   | name of shell script, which could be used to verify remote node before starting |
| ssh_init   | name of shell script, which will be called before starting dabc_exe |
| workdir    | name of directory where dabc_exe should start |
| copycfg    | if true, config file will be copied to the configured working directory of the process |
| LD_LIBRARY_PATH | additional paths, used to search for libraries |
| debugger   | "true" "false" to enable/disable debugger usage. One also can directly specify command prefix like "gdb -x run.txt --args" |
| taskset    | taskset arguments like "-c 10-15" to set affinity of whole dabc_exe process. See [here](@ref dabc_affinity) for more details |
| affinity   | affinity mask which defines how threads code be used. See [here](@ref dabc_affinity) for more details |
| threads_layout  | "minimal", "permodule", "balanced" (default), "maximal" |
| thrdstoptime  | timeout when stopping thread in destructor, default 5 sec |
| stdout     | redirection for standard output, for instance file name |
| errout     | redirection for error output, for instance file name |
| nullout    | if true, all output will be redirected ro /dev/null |
| slow-time  | which methods used for time measuremens, default is true |


### Memory pool

The typical configuration for memory pool look like this.

~~~~~~~~~~~~~~~~~~~~~{.xml}
...
    <MemoryPool name="Pool">
       <BufferSize value="65536"/>
       <NumBuffers value="100"/>
    </MemoryPool>
...
~~~~~~~~~~~~~~~~~~~~~

Following parameters could be specified:

| Parameter  | Description |
| --------:  | :---------- |
| BufferSize | Size of buffer in bytes |
| NumBuffers | Number of buffers |
| RefCoeff   | Ratio between number of references and buffers number (default 2) |
| NumSegments | Number of segments in preallocated list (default 8) |
| Alignment | Alignment of memory buffer in bytes (default 16) |


### Thread

Following parameters can be specified for each thread separately:

| Parameter  | Description |
| --------:  | :---------- |
| thrdstoptime  | timeout when stopping thread in destructor, default 5 sec |
| affinity  | thread affinity, see appropriate section in introduction |


### Module

Typical configuration for module could look like this:

~~~~~~~~~~~~~~~~~~~~~{.xml}
...
 <Module name="Repeat" class="dabc::RepeaterModule">
    <NumInputs value="1"/>
    <NumOutputs value="1"/>
    <PoolName value="Pool"/>

    <InputPort name="Input0" url="lmd://Generator"/>
    <OutputPort name="Output0" url="mbs://Stream"/>
 </Module>
...
~~~~~~~~~~~~~~~~~~~~~

Following attributes can be specified:

| Attribute  | Description |
| --------:  | :---------- |
| name       | Module name |
| class      | Module class, which is necessary only when automatic module creation is used |
| thread     | Thread name, to which module must be assigned |


Following parameter can be specified

| Parameter  | Description |
| --------:  | :---------- |
| NumInputs  | number of inputs in the module (default 0) |
| NumOutputs | number of outputs in the module (default 0) |
| PoolName   | name of memory pool, used as source of buffers (default empty) |


### Port

Typical configuration for input and output ports look like:

~~~~~~~~~~~~~~~~~~~~~{.xml}
...
 <Module name="Repeat" class="dabc::RepeaterModule">

    <InputPort name="Input0" url="lmd://Generator"/>
    <OutputPort name="Output0" url="mbs://Stream"/>
 </Module>
...
~~~~~~~~~~~~~~~~~~~~~

Following parameters of attributes could be specified:

| Parameter  | Description |
| --------:  | :---------- |
| name       | port name |
| url        | used to create transport for the port |
| urlopt     | extra options for url string, can be specified in other places |
| queue      | queue size, used for the port |
| thread     | thread name, used to create transport |
| loop       | maximumal event loop length for port before it will be interrupted (default 0 - endless) |
| bind       | name of bind port |
| rate       | name of rate parameter |
| signal     | which events delivered to user. Can be "none", "every", "confirm" (default) |
| reconnect  | period of automatic reconnect (default -1 - disabled) |
| numreconn  | number of reconnect attempts (default - 10) |
| onerror    | action done in case of error "close", "none", "stop", "exit", "abort" |


### Connections

Connection used during automatic creation and look like this:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Remote" host="lx-pool.gsi.de" port="3425">
...
     <Connection output="Module0/Output1" input="Module1/Input0"/>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

As __output__ and __input__ names of output and input port must be specified.
Port name can contains also node identifier, therefore connection with remote node can be
stablished as well. In this case syntax will looke like:

~~~~~~~~~~~~~~~~~~~~~{.xml}
...
     <Connection output="dabc://node1/Module0/Output1" input="Module1/Input0"/>
...
~~~~~~~~~~~~~~~~~~~~~

It is required, that at least one port (or both) situated in local node.


It is also possible to specify multiple connections with single `<Connectionin>` node.
There are typical situations, when connections between all application nodes must be established -
so calles "all-to-all" pattern. In this on each node module with __N__ outputs and
module with __N__ inputs should be present, where __N__ is number of nodes. To establish connections between
them, one should specify:

~~~~~~~~~~~~~~~~~~~~~{.xml}
...
     <Connection kind="all-to-all" output="OutputModule" input="InputModule"/>
...
~~~~~~~~~~~~~~~~~~~~~

In such case  __output__ and __input__ should be modules names; it can be the same module.


Here is complete list of attributes:

| Attribute  | Description |
| --------:  | :---------- |
| kind       | normal (default) - connection between two ports, all-to-all - connection between all nodes |
| output     | Name of output (port or module) |
| input      | Name of input (port or module) |
| thread     | thread used to run connection |
| useackn    | Is acknowledge should be used  |
| optional   | If true, module could run alo when connection does not established  |
| device     | device name, which should be used to create connection  |
| timeout    | timeout to establish connection  |


### Application

Example:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Remote" host="lx-pool.gsi.de" port="3425">
     <Application class="UserApplication"/>
        <UserParameter1 value="12.37"/>
        <UserParameter2 value="abc_xyz"/>
        ...
     </Application>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

Inside application node one can put any user-specific parameters.
Also memory pools, devices, thread, modules  and others objects configuration,
created by application, can be placed inside application node.



## Environment variables

It is allowed to used all shell variables with the syntax:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Remote" host="lx-pool.gsi.de" port="3425">
     <Run>
       <lib value="libDabcMbs.so"/>
       <logfile value="Node${DABCNODEID}.log"/>
     </Run>
...
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

Also there are variables defined in the dabc process itself.

Following variables defined inside dabc itself.

| Variable     | Description |
| --------:    | :---------- |
| DABCSYS      | top directory of dabc installation |
| DABCUSERDIR  | user-specified directory |
| DABCWORKDIR  | current working directory |
| DABCNUMNODES | number of `<Context>` nodes in configuration file |
| DABCNODEID   | sequence number of current `<Context>` node in configuration file |


## Automatic creation

If no init function and no application class was set, dabc will try to create objects,
based on the content of xml file. These objects are:

   - Device
   - Thread
   - MemoryPool
   - Module
   - Ports connections

For every of these items one can specify attribute auto="false" to disable automatic creation of such object,
even it is present in xml file.


## Wildcard rules

For most objects one can specify name with wildcards like:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Example">
    <Run>
      <lib value="libDabcMbs.so"/>
    </Run>
    <MemoryPool name="Pool">
       <BufferSize value="65536"/>
       <NumBuffers value="100"/>
    </MemoryPool>
    <Module name="Module0" class="dabc::RepeaterModule"/>
    <Module name="Module1" class="dabc::RepeaterModule"/>
    <Module name="Module*">
       <NumInputs value="1"/>
       <NumOutputs value="1"/>
    </Module>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

In this example two modules are created.
And for both of them same configuration parameters will be used.

One could have many such multicast definitions, first matching will be used.


## Multi-node application

XMl file can contain several Context nodes like:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Example1" host="node1">
     ...
  </Context>
  <Context name="Example2" host="node2">
     ...
  </Context>
  <Context name="Example3" host="node3">
     ...
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~


In this case to start such application, __dabc_run__ shell script should be used in form:

    [shell] dabc_run Exmaple.xml run


One can use wildcard rules to set common parameters for some contextes:

~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="Example1" host="node1"/>
  <Context name="Example2" host="node2"/>
  <Context name="Example3" host="node3"/>
  <Context name="Example*">
    <Run>
      <lib value="libDabcMbs.so"/>
    </Run>
    <MemoryPool name="Pool">
       <BufferSize value="65536"/>
       <NumBuffers value="100"/>
    </MemoryPool>
    <Module name="Module0" class="dabc::RepeaterModule">
       <NumInputs value="1"/>
       <NumOutputs value="1"/>
    </Module>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~

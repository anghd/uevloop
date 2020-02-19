

# µEvLoop ![C/C++ CI](https://github.com/andsmedeiros/uevloop/workflows/C/C++%20CI/badge.svg?event=push)

A fast and lightweight event loop aimed at embedded platforms in C99.

## About

µEvLoop is a microframework build around a lightweight event loop. It provides the programmer the building blocks to put together async, interrupt-based systems.

µEvLoop is loosely inspired on the Javascript event loop and aims to provide a similar programming model. Many similar concepts, such as events and closures are included.
It is aimed at environments with very restricted resources, but can be run on all kinds of platforms.

### *DISCLAIMER*

µEvLoop is in its early days and the API may change at any moment for now. Although it's well tested, use it with caution. Anyway, feedback is most welcome.

## Highlights

* As minimalist and loose-coupled as possible.
* Does not allocate any dynamic memory on its own. All static memory needed is either allocated explicitly by the user or implicitly by [containers](#containers).
* Small memory footprint and runtime latency.
* Does not try to make assumptions about the underlying system.
* Depends only on a very small subset of the standard libc, mostly for fixed size integers and booleans.

## API documentation

The API documentation is automatically generated by Doxygen. Find it [here](https://andsmedeiros.github.io/uevloop/).

## Testing

Tests are written using a simple set of macros. To run them, execute `make test`.

Please note that the makefile shippped is meant to be run in modern Linux systems. Right now, it makes use of bash commands and utilities as well as expects libSegFault.so to be in a hardcoded path.

If this doesn't fit your needs, edit it as necessary.

### Test coverage

To generate code coverage reports, run `make coverage`. This requires `gcov`, `lcov` and `genhtml` to be on your `PATH`. After running, the results can be found on `uevloop/coverage/index.html`.

## Core data structures

These data structures are used across the whole framework. They can also be used by the programmer in userspace as required.

Core data structures can be found on the `src/utils` directory.

**All core data structures are unsafe. Be sure to wrap access to them in [critical sections](#critical-sections) if you mean to share them amongst contexts asynchronous to each other.**

### Closures

A closure is an object that binds a function to some context. When invoked with arbitrary parameters, the bound function is called with both context and parameters available. With closures, some very powerful programming patterns, as functional composition, becomes way easier to implement.

As closures are somewhat light, it is often useful to pass them around by value.

#### Basic closure usage

```C
  #include <stdlib.h>
  #include <stdint.h>
  #include "utils/closure.h"

  static void *add(closure_t *closure){
    uintptr_t value1 = (uintptr_t)closure->context;
    uintptr_t value2 = (uintptr_t)closure->params;

    return (void *)(value1 + value2);
  }

  // ...

  // Binds the function `add` to the context (5)
  closure_t add_five = closure_create(&add, (void *)5, NULL);

  // Invokes the closure with the parameters set to (2)
  uintptr_t result = (uintptr_t)closure_invoke(&add_five, (void *)2);
  // Result is 7

```

#### *A word on (void \*)*

Closures take the context and parameters as a void pointer and return the same. This is meant to make possible to pass and return complex objects from them.

At many times, however, the programmer may find the values passed/returned are small and simple (*i.e.*: smaller than a pointer). If so, it is absolutely valid to cast from/to a `uintptr_t` or other data type known to be at most the size of a pointer. The above example does that to avoid creating unnecessary object pools or allocating dynamic memory.

### Circular queues

Circular queues are fast FIFO (*first-in-first-out*) structures that rely on a pair of indices to maintain state. As the indices are moved forward on push/pop operations, the data itself is not moved at all.

The size of µEvLoop's circular queues are **required** to be powers of two, so it is possible to use fast modulo-2 arithmetic. As such, on queue creation, the size **must** be provided in its log2 form.

***FORGETTING TO SUPPLY THE QUEUE'S SIZE IN LOG2 FORM MAY CAUSE THE STATIC ALLOCATION OF GIANT MEMORY POOLS***

#### Basic circular queue usage

```c
#include <stdlib.h>
#include <stdint.h>
#include "utils/circular-queue.h"

#define BUFFER_SIZE_LOG2N   (5)
#define BUFFER_SIZE         (1<<BUFFER_SIZE_LOG2N)  // 1<<5 == 2**5 == 32

// ...

cqueue_t queue;
void *buffer[BUFFER_SIZE];
// Created a queue with 32 (2**5) slots
cqueue_init(&queue, buffer, BUFFER_SIZE_LOG2N);

// Push items in the queue
cqueue_push(&queue, (void *)3);
cqueue_push(&queue, (void *)2);
cqueue_push(&queue, (void *)1);

// Pop items from the queue
uintptr_t value1 = (uintptr_t)cqueue_pop(&queue); // value1 is 3
uintptr_t value2 = (uintptr_t)cqueue_pop(&queue); // value2 is 2
uintptr_t value3 = (uintptr_t)cqueue_pop(&queue); // value3 is 1
```

Circular queues store void pointers. As it is the case with closures, this make possible to store complex objects within the queue, but often typecasting to an smaller value type is more useful.

### Object pools

On embedded systems, hardware resources such as processing power or RAM memory are often very limited. As a consequence, dynamic memory management can become very expensive in both aspects.

Object pools are statically allocated arrays of objects whose addresses are stored in a queue. Whenever the programmer needs a object in runtime, instead of dynamically allocating memory, it is possible to simply pop an object pointer from the pool and use it away.

Because object pools are statically allocated and backed by [circular queues](#circular-queues), they are very manageable and fast to operate.

#### Basic object pool usage

```c
#include <stdlib.h>
#include <stdint.h>
#include "utils/object-pool.h"

typedef struct obj obj_t;
struct obj {
  uint32_t num;
  char str[32];
  // Whatever
};

// ...

// The log2 of our pool size.
#define POOL_SIZE_LOG2N   (5)
DECLARE_OBJPOOL_BUFFERS(obj_t, POOL_SIZE_LOG2N, my_pool);

objpool_t my_pool;
objpool_init(&my_pool, POOL_SIZE_LOG2N, sizeof(obj_t), OBJPOOL_BUFFERS(my_pool));
// my_pool now is a pool with 32 (2**5) obj_t

// ...

// Whenever the programmer needs a fresh obj_t
obj_t *obj = (obj_t *)objpool_acquire(&my_pool);

// When it is no longer needed, return it to the pool
objpool_release(&my_pool, obj);
```

### Linked lists

µEvLoop ships a simple linked list implementation thar holds void pointers, as usual.

#### Basic linked list usage

```c
#include <stdlib.h>
#include <stdint.h>
#include "utils/linked-list.h"

// ...

llist_t list;
llist_init(&list);

llist_node_t nodes[2] = {
  {(void *)1, NULL},
  {(void *)2, NULL}
};

// Push items into the list
llist_push_head(&list, &nodes[0]);
llist_push_head(&list, &nodes[1]);

// List now is TAIL-> [1]-> [2]-> NULL. HEAD-> [2]
llist_node_t *node1 = (llist_node_t *)llist_pop_tail(&list);
llist_node_t *node2 = (llist_node_t *)llist_pop_tail(&list);

//node1 == nodes[0] and node2 == nodes[1]
```

## Containers
Containers are objects that encapsulate declaration, initialisation and manipulation of core data structures used by the framework.

They also encapsulates manipulation of these data structures inside [critical sections](#critical-sections), ensuring safe access to shared resources across the system.

### System pools

The `syspools` module is a container for the system internal object pools. It contains pools for events and linked list nodes used by the core modules.

The system pools module is meant to be internally operated only. The only responsibility of the programmer is to allocate, initialise and provide it to other core modules.

To configure the size of each pool created, edit `src/config.h`.

#### System pools usage

```c
#include "system/containers/system-pools.h"

// ...

syspools_t pools;
syspools_init(&pools);
// This allocates two pools:
//   1) pools.event_pool
//   2) pools.llist_node_pool
```

### System queues

The `sysqueues` module contains the necessary queues for sharing data amongst the core modules. It holds queues for events in differing statuses.

As is the case with system pools, the `sysqueues` module should not be directly operated by the programmer, except for declaration and initialisation.

Configure the size of each queue created in `src/config.h`.


#### System queues usage

```C
#include "system/containers/system-queues.h"

// ...

sysqueues_t queues;
sysqueues_init(&queues);
// This allocates two queues:
//   1) queues.event_queue (events ready to be processed are put here)
//   2) queues.schedule_queue (events ready to be scheduled are put here)
```

### Application

The `application` module is a convenient toplevel container for all the internals of an µEvLoop'd app. It is not necessary at all but contains much of the boilerplate in a typical application.

It also proxies functions to the [`event loop`](#event-loop) and [`scheduler`](#scheduler) modules, serving as a single point entry for the system operation.

The following code is a realistic minimal setup of the framework.
```c
#include "system/containers/application.h"
#include <stdint.h>

static volatile uint32_t counter = 0;
static application_t my_app;

// 1 kHz timer
void my_timer_isr(){
  my_timer_isr_flag = 0;
  app_update_timer(&my_app, ++counter);
}

int main (int argc, char *argv[]){
  app_init(&my_app);

  // Start scheduling timers through my_app.scheduler or enqueuing
  // closures through my_app.event_loop
  // Create modules that emit signals and listen to then from here

  while(1){  
    app_tick(&my_app);
  }

  return 0;
}
```

## Core modules

### Scheduler

The scheduler is a module that keeps track of current execution time and closures to be run in the future. It provides similar functionality to the `setTimeout` and `setInterval` Javascript functions.

Two queues lead in and out of it: the inbound schedule_queue is externally fed events that should be scheduled and then accounted for; the outbound event_queue hold events that are due to be collected and processed.

This module needs access to system's pools and queues.

#### Basic scheduler initialisation

```c
#include "system/containers/system-pools.h"
#include "system/containers/system-queues.h"
#include "system/scheduler.h"

// ...

// Create the system containers
syspools_t pools;
syspools_init(&pools);
sysqueues_t queues;
sysqueues_init(&queues);

// Create the scheduler
scheduler_t scheduler;
sch_init(&scheduler, &pools, &queues);
```

#### Scheduler operation

The `scheduler` module accepts input of closures and scheduling info an then turns it into a timer event. This timer is then inserted in a timer list, which is sorted by each timer's due time.

```c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils/closure.h"

static void *print_num(closure_t *closure){
  uintptr_t num = (uintptr_t)closure->context;
  printf("%d\n", num);

  return NULL;
}

// ...

closure_t  print_one = closure_create(&print_num, (void *)1, NULL);
closure_t  print_two = closure_create(&print_num, (void *)2, NULL);
closure_t  print_three = closure_create(&print_num, (void *)3, NULL);

// Schedules to run 1000ms in the future.
sch_run_later(&scheduler, 1000, print_one);

// Schedules to run at intervals of 500ms, runs the first time after 500ms
sch_run_at_intervals(&scheduler, 500, false, print_two);

// Schedules to run at intervals of 300ms, runs the first time the next runloop
sch_run_at_intervals(&scheduler, 300, true, print_three);
```

The `scheduler` must be fed regularly to work. It needs both an update on the running time as an stimulus to process enqueued timers. Ideally, a hardware timer will be consistently incrementing a counter and feeding it at an ISR while in the main loop the scheduler is oriented to process its queue.

```c
// millisecond counter
volatile uint32_t counter = 0;

// 1kHz timer ISR
void my_timer_isr(){
  my_timer_isr_flag = 0;
  sch_update_timer(&scheduler, ++counter);
}

// ...

// On the main loop
sch_manage_timers(&scheduler);
```

When the function `sch_manage_timers` is called, two things happen:
1. The `schedule_queue` is flushed  and every timer in it is scheduled accordingly;
2. The scheduler iterates over the scheduled timer list from the beginning and breaks it when it finds a timer scheduled further in the future. It then proceeds to move each timer from the extracted list  to the `event_queue`, where they will be further collected and processed.

#### Scheduler time resolution

There are two distinct factors that will determine the actual time resolution of the scheduler:
1. the frequency of the feed in timer ISR
2.  the frequency the function `sch_manage_timers` is called.

The basic resolution variable is the feed-in timer frequency. Having this update too sporadically will cause events scheduled to differing moments to be indistinguishable regarding their schedule (*e.g.*: most of the time, having the timer increment every 100ms will make any events scheduled to moments with less than 100ms of difference to each other to be run in the same runloop).

A good value for the timer ISR frequency is usually between 1 kHz - 200 Hz, but depending on project requirements and available resources it can be less. Down to around 10 Hz is still valid, but precision will start to deteriorate quickly from here on.

There is little use having the feed-in timer ISR run at more than 1 kHz, as it is meant to measure milliseconds. Software timers are unlikely to be accurate enough for much greater frequencies anyway.

If the `sch_manage_timers` function is not called frequently enough, events will start enqueuing and won't be served in time. Just make sure it is called when the counter is updated or when there are events on the schedule queue.

### Event loop

The central piece of µEvLoop (even its name is a bloody reference to it) is the event loop, a queue of events to be processed sequentially. It is not aware of the execution time and simply process all enqueued events when run. Most heavy work in the system happens here.

The event loop requires access to system's internal pools and queues.

#### Basic event loop initialisation

```c
#include "system/containers/system-pools.h"
#include "system/containers/system-queues.h"
#include "system/event-loop.h"

// Create system containers
syspools_t pools;
syspools_init(&pools);
sysqueues_t queues;
sysqueues_init(&queues);

// Create the event loop
evloop_t loop;
evloop_init(&loop, &pools, &queues);
```

#### Event loop usage

The event loop is mean to behave as a run-to-completion task scheduler. Its `evloop_run` function should be called as often as possible as to minimise execution latency. Each execution of `evloop_run` is called a *runloop* .

The only way the programmer interacts with it, besides creation / initialisation, is by enqueuing hand-tailored closures directly, but other system modules operate on the event loop behind the stage.

Any closure can be enqueued multiple times.

```c
#include <stdlib.h>
#include "utils/closure.h"

// ...

static void *increment(closure_t *closure){
  uintptr_t *value = (uintptr_t *)closure->context;
  (*value)++;

  return NULL;
}

// ...

uintptr_t value = 0;
closure_t closure = closure_create(&increment, (void *)&value, NULL);

evloop_enqueue_closure(&loop, &closure);
// value is 0

evloop_run(&loop);  
// value is 1

evloop_enqueue_closure(&loop, &closure);
evloop_enqueue_closure(&loop, &closure);
evloop_run(&loop);
// value is 3

```
***WARNING!*** `evloop_run` is the single most important function within µEvLoop. Almost every other core module depends on the event loop and if this function is not called, the loop won't work at all. Don't ever let it starve.

 ### Signal

 Signals are similar to events in Javascript. It allows the programmer to message distant parts of the system to communicate with each other in a pub/sub fashion.

 At the centre of the signal `system` is the Signal Relay, a structure that bind speciffic signals to its listeners. When a signal is emitted, the relay will **asynchronously** run each listener registered for that signal. If the listener was not recurring, it will be destroyed upon execution by the event loop.

 #### Signals and relay initialisation

To use signals, the programmer must first define what signals will be available in a particular relay, then create the relay bound to this events.

To be initialised, the relay must have access to the system's internal pools and queues. The programmer will also need to supply it a buffer of [linked lists](#linked-lists), where listeners will be stored.

 ```c
#include "system/containers/system-pools.h"
#include "system/containers/system-queues.h"
#include "system/signal.h"
#include "utils/linked-list.h"

// Create the system containers
syspools_t pools;
syspools_init(&pools);
sysqueues_t queues;
sysqueues_init(&queues);

// Define what signals will be available to this relay.
// Doing so in an enum makes it easy to add new signals in the future.
enum my_module_signals {
  SIGNAL_1 = 0,
  SIGNAL_2,
  SIGNAL_COUNT
};

// Declare the relay buffer. Note this array will be the number of signals large.
llist_t buffer[SIGNAL_COUNT];

// Create the relay
signal_relay_t relay;
void signal_relay_init(&relay, &pools, &queues, buffer, SIGNAL_COUNT);
 ```

 #### Signal operation

 ```c
// This is the listener function.
static void *respond_to_signal(closure_t *closure){
  uintptr_t num = (uintptr_t)closure->context;
  // Signals can be emitted with parameters, just like events in JS
  char c = (char)(uintptr_t)closure->params;
  printf("%d%c\n", num, c);

  return NULL;
}

// Listeners can be persistent. They will fire once each time the signal is emitted
closure_t respond_to_signal_1 = closure_create(&respond_to_signal, (void *)1, NULL);
signal_listener_t listener_1 =
  signal_listen(SIGNAL_1, &relay, &respond_to_signal_1);

// Listeners can also be transient, so they fire ust on first emission
closure_t respond_to_signal_2 = closure_create(&respond_to_signal, (void *)2, NULL);
signal_listener_t listener_2 =
  signal_listen_once(SIGNAL_2, &relay, &respond_to_signal_2);

// ...

signal_emit(SIGNAL_1, &relay, (void *)('a')); // prints 1a
signal_emit(SIGNAL_2, &relay, (void *)('b')); // prints 2b
signal_emit(SIGNAL_1, &relay, (void *)('c')); // prints 1c
signal_emit(SIGNAL_2, &relay, (void *)('d')); // doesn't print anything
```

Please note the listener function will not be executed immediately, despite what this last snippet can lead to believe. Internally, each closure will be sent to the event loop and only when it runs will the closures be invoked.

You can also unlisten for events. This will prevent the listener returned by a `signal_listen()` or `signal_listen_once()` operation to have its closure invoked when the [event loop](#event-loop) performs the next runloop.
Additionally, said listener will be removed from the signal vector on such opportunity.

```c
  signal_unlisten(listener_1, &relay);
  signal_unlisten(listener_2, &relay);  // This has no effect because the listener
                                        // for SIGNAL_2 has already been marked as unlistened
```

## Concurrency model

µEvLoop is meant to run baremetal, primarily in simple single-core MCUs. That said, nothing stops it from being employed as a side library in RTOSes or in full-fledged x86_64 multi-threaded desktop applications.

Communication between asynchronous contexts, such as ISRs and side threads, is done through some shared data structures defined inside the library's core modules. As whenever dealing with non-atomic shared memory, there must be sincronisation between accesses to these structures as to avoid memory corruption.

µEvLoop does not try to implement a universal locking scheme fit for any device. Instead, some generic critical section definition is provided.

### Critical sections

By default, critical sections in µEvLoop are a no-op. They are provided as a set of macros that can be overriden by the programmer to implement platform speciffic behaviour.

For instance, while running baremetal it may be only necessary to disable interrupts to make sure accesses are synchronised. On a RTOS multi-threaded environment, on the other hand, it may be necessary to use a mutex.


There are three macros that define critical section implementation:

1. `UEVLOOP_CRITICAL_SECTION_OBJ_TYPE`

  If needed, a global critical section object can be declared. If this macro is defined, this object will be available to any critical section under the symbol `uevloop_critical_section`.

  The `UEVLOOP_CRITICAL_SECTION_OBJ_TYPE` macro defines the **type** of the object. It is the programmer's responsibility to declare the globally allocate and initialise the object.

2. `UEVLOOP_CRITICAL_ENTER`

  Enters a new critical section. From this point until the critical section exits, no other thread or ISR may attempt to access the system's shared memory.

3. `UEVLOOP_CRITICAL_EXIT`

  Exits the current critical section. After this is called, any shared memory is allowed to be claimed by some party.

## Motivation

I often work with small MCUs (8-16bits) that simply don't have the necessary power to run a RTOS ou any fancy scheduling solution. Right now I am working on a new comercial project and felt the need to build something by my own. µEvLoop is my bet on how a modern, interrupt-driven and predictable embedded application should be.
I am also looking for a new job and needed to improve my portifolio.

## Roadmap
* ~~Correct some bugs I am aware of~~
* ~~Comment code and generate API doc~~
* Make timer events cancellable / pausable / resumable
* Better error handling

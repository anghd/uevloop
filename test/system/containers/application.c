#include "application.h"

#include <stdlib.h>
#include "system/containers/application.h"
#include "../../uelt.h"

#define DECLARE_APP()                                                           \
    uel_application_t app;                                                          \
    uel_app_init(&app);

static char *should_init_app(){
    DECLARE_APP();

    uelt_assert_pointers_equal(
        "app.pools.event_pool.buffer",
        app.pools.event_pool_buffer,
        app.pools.event_pool.buffer
    );
    uelt_assert_pointers_equal(
        "app.pools.llist_node_pool.buffer",
        app.pools.llist_node_pool_buffer,
        app.pools.llist_node_pool.buffer
    );
    uelt_assert_pointers_equal(
        "app.queues.event_queue.buffer",
        app.queues.event_queue_buffer,
        app.queues.event_queue.buffer
    );
    uelt_assert_pointers_equal(
        "app.queues.schedule_queue.buffer",
        app.queues.schedule_queue_buffer,
        app.queues.schedule_queue.buffer
    );
    uelt_assert_pointers_equal(
        "app.scheduler.pools",
        &app.pools,
        app.scheduler.pools
    );
    uelt_assert_pointers_equal(
        "app.scheduler.queues",
        &app.queues,
        app.scheduler.queues
    );
    uelt_assert_pointers_equal(
        "app.event_loop.pools",
        &app.pools,
        app.event_loop.pools
    );
    uelt_assert_pointers_equal(
        "app.event_loop.queues",
        &app.queues,
        app.event_loop.queues
    );
    uelt_assert_pointers_equal(
        "app.relay.queues",
        &app.queues,
        app.relay.queues
    );
    uelt_assert_pointers_equal(
        "app.relay.pools",
        &app.pools,
        app.relay.pools
    );
    uelt_assert_pointers_equal(
        "app.relay.signal_vector",
        app.relay_buffer,
        app.relay.signal_vector
    );
    uelt_assert_ints_equal("app.relay.width", UEL_APP_EVENT_COUNT, app.relay.width);
    uelt_assert("app.run_scheduler must had been set", app.run_scheduler);
    return NULL;
}

static char *should_update_timer(){
    DECLARE_APP();

    uelt_assert_int_zero("app.scheduler.timer", app.scheduler.timer);

    uel_app_update_timer(&app, 10);
    uelt_assert_ints_equal(
        "app.scheduler.timer after first set",
        10,
        app.scheduler.timer
    );

    uel_app_update_timer(&app, 100);
    uelt_assert_ints_equal(
        "app.scheduler.timer after first set",
        100,
        app.scheduler.timer
    );

    return NULL;
}

static char *should_set_uel_scheduer_run_flag(){
    DECLARE_APP();

    uel_app_tick(&app);
    uelt_assert_not("app.run_scheduler must had been unset", app.run_scheduler);

    uel_app_update_timer(&app, 100);
    uelt_assert("app.run_scheduler must had been set", app.run_scheduler);

    uel_app_tick(&app);
    uelt_assert_not("app.run_scheduler must had been unset", app.run_scheduler);

    uel_app_tick(&app);
    uelt_assert_not("app.run_scheduler must had been unset", app.run_scheduler);

    return NULL;
}

static void *increment(uel_closure_t *closure){
    uintptr_t *counter = (uintptr_t *)closure->context;
    (*counter)++;

    return NULL;
}
static char *should_tick(){
    DECLARE_APP();

    uintptr_t counter1 = 0, counter2 = 0, counter3 = 0;

    uel_closure_t closure1 = uel_closure_create(&increment, (void *)&counter1, NULL);
    uel_closure_t closure2 = uel_closure_create(&increment, (void *)&counter2, NULL);
    uel_closure_t closure3 = uel_closure_create(&increment, (void *)&counter3, NULL);

    uel_evloop_enqueue_closure(&app.event_loop, &closure1);
    uel_sch_run_later(&app.scheduler, 100, closure2);
    uel_sch_run_at_intervals(&app.scheduler, 100, true, closure3);

    uel_app_tick(&app);
    uelt_assert_ints_equal("counter1 at 0ms", 1, counter1);
    uelt_assert_int_zero("counter2 at 0ms", counter2);
    uelt_assert_ints_equal("counter3 at 0ms", 1, counter3);

    uel_app_update_timer(&app, 50);
    uel_app_tick(&app);
    uelt_assert_ints_equal("counter1 at 50ms", 1, counter1);
    uelt_assert_int_zero("counter2 at 50ms", counter2);
    uelt_assert_ints_equal("counter3 at 50ms", 1, counter3);

    uel_app_update_timer(&app, 100);
    uel_app_tick(&app);
    uelt_assert_ints_equal("counter1 at 100ms", 1, counter1);
    uelt_assert_ints_equal("counter2 at 100ms", 1, counter2);
    uelt_assert_ints_equal("counter3 at 100ms", 2, counter3);

    return NULL;
}

static void *nop(uel_closure_t *closure){
    return NULL;
}
static char *should_proxy_functions(){
    DECLARE_APP();

    uel_closure_t closure = uel_closure_create(&nop, NULL, NULL);

    uel_app_enqueue_closure(&app, &closure);
    uelt_assert_ints_equal(
        "uel_sysqueues_count_enqueued_events",
        1,
        uel_sysqueues_count_enqueued_events(&app.queues)
    );
    uelt_assert_ints_equal(
        "uel_sysqueues_count_scheduled_events",
        0,
        uel_sysqueues_count_scheduled_events(&app.queues)
    );

    uel_app_run_later(&app, 1000, closure);
    uelt_assert_ints_equal(
        "uel_sysqueues_count_enqueued_events",
        1,
        uel_sysqueues_count_enqueued_events(&app.queues)
    );
    uelt_assert_ints_equal(
        "uel_sysqueues_count_scheduled_events",
        1,
        uel_sysqueues_count_scheduled_events(&app.queues)
    );

    uel_app_run_at_intervals(&app, 500, false, closure);
    uelt_assert_ints_equal(
        "uel_sysqueues_count_enqueued_events",
        1,
        uel_sysqueues_count_enqueued_events(&app.queues)
    );
    uelt_assert_ints_equal(
        "uel_sysqueues_count_scheduled_events",
        2,
        uel_sysqueues_count_scheduled_events(&app.queues)
    );

    uel_app_run_at_intervals(&app, 500, true, closure);
    uelt_assert_ints_equal(
        "uel_sysqueues_count_enqueued_events",
        2,
        uel_sysqueues_count_enqueued_events(&app.queues)
    );
    uelt_assert_ints_equal(
        "uel_sysqueues_count_scheduled_events",
        2,
        uel_sysqueues_count_scheduled_events(&app.queues)
    );

    return NULL;
}

char *uel_app_run_tests(){

    uelt_run_test("should correctly initialise an application", should_init_app);
    uelt_run_test(
        "should correctly update an application internal timer",
        should_update_timer
    );
    uelt_run_test(
        "should correctly set the scheduler run flag when the timer is updated",
        should_set_uel_scheduer_run_flag
    );
    uelt_run_test(
        "should correctly tick an application event loop and operate accordingly",
        should_tick
    );
    uelt_run_test(
        "should correctly proxy scheduler and event loop functions",
        should_proxy_functions
    );

    return NULL;
}

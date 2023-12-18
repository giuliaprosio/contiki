#include "contiki.h"

#include <stdio.h> /* For printf() */
#include <time.h> /* For rand() */
#include <stdlib.h> /* For rand() */

#define SIZE 1

process_event_t queue_event;

static int queue_size = 0;
static int queue[SIZE] = {0};

// returns a random between 1 and 5
int get_rand() {
    return (rand() % 5) + 1;
}

/*---------------------------------------------------------------------------*/
PROCESS(producer, "producer");
PROCESS(consumer, "consumer");
AUTOSTART_PROCESSES(&consumer, &producer);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(producer, ev, data)
{
srand(time(NULL));

static struct etimer timer;

PROCESS_BEGIN();

/* Setup a periodic timer that expires after 2 seconds. */
const int rand_val = get_rand();
etimer_set(&timer, CLOCK_SECOND * rand_val);

while(1) {
/* Wait for the periodic timer to expire and then restart the timer. */
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

if (queue_size < SIZE) {
const int old_q_size = queue_size;
const int new_val = get_rand();
printf("producer: old queue size:  %d\n", queue_size);
printf("producer: pushing %d\n", new_val);
queue[queue_size++] = new_val;
printf("producer: new queue size:  %d\n", queue_size);

// notify consumer that a queue is not empty
if (old_q_size == 0) {
process_post(&consumer, queue_event, NULL);
}
} else {
printf("producer: waiting as queue is full\n");
PROCESS_WAIT_EVENT_UNTIL(ev == queue_event);
printf("producer: woke up as queue is not full anymore\n");
}

const int rand_val = get_rand();
etimer_set(&timer, CLOCK_SECOND * rand_val);
}

PROCESS_END();
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(consumer, ev, data)
{
PROCESS_BEGIN();

srand(time(NULL));
static struct etimer timer;

const int rand_val = get_rand();
etimer_set(&timer, CLOCK_SECOND * rand_val);

while(1) {
/* Sanity check */
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
if (queue_size > 0) {
const int old_q_size = queue_size;
printf("consumer: old queue size:  %d\n", queue_size);
printf("consumer: getting element from queue: %d\n", queue[queue_size-1]);
queue_size--;
printf("consumer: new queue size:  %d\n", queue_size);

//notify producer that queue is not full anymore
if (old_q_size == SIZE-1) {
process_post(&producer, queue_event, NULL);
}
} else {
printf("consumer: waiting as queue is empty\n");
PROCESS_WAIT_EVENT_UNTIL(ev == queue_event);
printf("consumer: woke up from producer as queue is not empty anymore\n");
}

const int rand_val = get_rand();
etimer_set(&timer, CLOCK_SECOND * rand_val);
}

PROCESS_END();
}
/*---------------------------------------------------------------------------*/


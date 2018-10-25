/**
 * \file
 *         Producer & consumer
 * \author
 *         Andrea Vicari <andrea.vicari@usi.ch>
 */

#include "contiki.h"
#include "stdlib.h"

#include "sys/log.h"
#define LOG_MODULE "Test"
#define LOG_LEVEL LOG_LEVEL_INFO

#include <stdio.h> /* For printf() */
#define QUEUE_SIZE 128

/*---------------------------------------------------------------------------*/
PROCESS(producer_process, "Producer process");
PROCESS(consumer_process, "Consumer process");
AUTOSTART_PROCESSES(&consumer_process, &producer_process);
/*---------------------------------------------------------------------------*/
static process_event_t producer_event;
static process_event_t consumer_event;
static int shared_queue[QUEUE_SIZE];
static int shared_queue_index = 0;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(producer_process, ev, data)
{

	PROCESS_BEGIN();

	/* Make sure the other process has time to start... */
	PROCESS_PAUSE();

	/* Start the producer - consumer... */
	producer_event = process_alloc_event();

	while (1)
	{
		if (ev == consumer_event)
		{
			if (shared_queue_index < QUEUE_SIZE - 1)
			{
				shared_queue[shared_queue_index] = shared_queue_index;
				LOG_INFO("\033[01;33mThe producer produced the value: %d.\n\033[0m", shared_queue_index);
				shared_queue_index++;

				if (shared_queue_index >= 1)
				{
					LOG_INFO("\033[01;33mThe producer stopped writing.\n\n\033[0m");
					process_post(&consumer_process, producer_event, 0);
					LOG_INFO("\033[1;31mActual size of the queue: %d. \n\n\033[0m", shared_queue_index);
				}

				if (rand() % 8 > 2)
				{
					PROCESS_PAUSE();
				}
			}
			else
			{
				LOG_INFO("\033[01;33mThe producer found the queue full. Nothing to do.\n\n\033[0m");
				PROCESS_WAIT_EVENT();
			}
		}
		else
		{
			PROCESS_WAIT_EVENT();
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(consumer_process, ev, data)
{
	PROCESS_BEGIN();

	PROCESS_PAUSE();
	consumer_event = process_alloc_event();
	process_post(&producer_process, consumer_event, 0);

	while (1)
	{

		if (ev == producer_event)
		{

			if (shared_queue_index > 0)
			{
				LOG_INFO("\033[1;34mThe consumer consumed the element: %d.\n\033[0m", shared_queue[shared_queue_index-1]);
				shared_queue_index--;

				if (shared_queue_index <= QUEUE_SIZE - 1)
				{
					LOG_INFO("\033[1;34mThe consumer stopped reading from the queue.\n\n\033[0m");
					process_post(&producer_process, consumer_event, 0);
					LOG_INFO("\033[1;31mActual size of the queue: %d. \n\n\033[0m", shared_queue_index);
				}

				if (rand() % 8 > 2)
				{
					PROCESS_PAUSE();
				}
			}
			else
			{
				LOG_INFO("\033[1;34mThe consumer found the queue empty. Nothing to do.\n\n\033[0m");
				PROCESS_WAIT_EVENT();
			}			
		}
		else
		{
			PROCESS_WAIT_EVENT();
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*
 * queue.h
 *
 *  Created on: Apr 21, 2017
 *      Author: magic
 */

#ifndef SRC_QUEUE_INTERNAL_H_
#define SRC_QUEUE_INTERNAL_H_

#define TAILQ_FIRST(head)	((head)->tqh_first)
#define TAILQ_END(head)		NULL
#define TAILQ_NEXT(elm, field)	((elm)->field.tqe_nex)
#define TAILQ_LAST(head, headname)	\
	(*(((struct headname* )(head)->tqh_last))->tqh_last)

#define TAILQ_INIT(head) do {	\
	(head)->tqh_first = NULL;	\
	(head)->tqh_next = &(head)->first;\
} while (0)

#define TAILQ_REMOVE(head, elm, field) do {				\
	if (((elm)->field.tqe_next) != NULL)				\
		(elm)->field.tqe_next->field.tqe_prev =			\
		    (elm)->field.tqe_prev;				\
	else								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
	*(elm)->field.tqe_prev = (elm)->field.tqe_next;			\
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.tqe_next = NULL;					\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &(elm)->field.tqe_next;			\
} while (0)

#define TAILQ_FOREACH(var, head, field)	\
	for ((var) = TAILQ_FIRST(head); (var) != TAILQ_END(head); var = TAILQ_NEXT(var, faild))

#endif /* SRC_QUEUE_INTERNAL_H_ */

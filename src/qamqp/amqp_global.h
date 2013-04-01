#ifndef qamqp_global_h__
#define qamqp_global_h__

#include <QtCore/qglobal.h>

#define QAMQP_P_INCLUDE
#define AMQPSCHEME "amqp"
#define AMQPSSCHEME "amqps"
#define AMQPPORT 5672
#define AMQPHOST "localhost"
#define AMQPVHOST "/"
#define AMQPLOGIN "guest"
#define AMQPPSWD  "guest"
#define FRAME_MAX 131072

#define QAMQP_VERSION "0.2.0"



#define AMQP_CONNECTION_FORCED 320

#define P_DECLARE_PRIVATE(Class) \
	friend class Class##Private; \
	inline Class##Private* pd_func() { return reinterpret_cast<Class##Private *>(this->pd_ptr); } \
	inline const Class##Private* pd_func() const { return reinterpret_cast<const Class##Private *>(this->pd_ptr); } 	
	

#define P_DECLARE_PUBLIC(Class) \
	inline Class* pq_func() { return static_cast<Class *>(this->pq_ptr); } \
	inline const Class* pq_func() const { return static_cast<const Class *>(this->pq_ptr); } \
	friend class Class; 


#define P_D(Class) Class##Private * const d = this->pd_func()
#define P_Q(Class) Class * const q = this->pq_func()

#endif // qamqp_global_h__

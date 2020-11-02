#ifndef SB_LIB_WATCHDOG_H
#define SB_LIB_WATCHDOG_H

#if defined LINUX_TARGET || defined RPI_TARGET 

/* Include Files */
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdexcept>
#include <string>

/* Typedefs */
typedef int(* WATCHDOG_ENTRY_FUNCTION)(void *, std::ostream *output_stream , std::ostream *err_stream );

namespace SB
{

// Utility classes for signal handling
class signal_exception : public std::runtime_error::runtime_error
{
public:
   signal_exception(const std::string& _message)
      : std::runtime_error(_message)
   {}
};

class signal_handler
{
    private:
        static signal_handler *p_handler;

    public:
        signal_handler(){ p_handler = this; m_exit_signal = false;}
        virtual ~signal_handler(){}
        
        bool get_exit_signal()
        {
            return m_exit_signal;
        }
        
        void set_exit_signal(bool exit_signal)
        {
            m_exit_signal = exit_signal;
        }

        static void exit_signal_handler(int ignored)
        {
            p_handler->set_exit_signal(true);
        }

        virtual void setup_signal_handlers(void (*handler)(int) = signal_handler::exit_signal_handler)
        {
            if (signal(SIGINT, handler) == SIG_ERR)
            {
                throw signal_exception("Error setting up signal SIGINT handlers!");
            }

            if (signal(SIGTERM, handler) == SIG_ERR)
            {
                throw signal_exception("Error setting up signal SIGTERM handlers!");
            }
        }

    protected:
        bool m_exit_signal;

};


/* Class Declarations */
class Watchdog
{
public:

    typedef enum
    {
        OK,
        DELIBERATELY_TERMINATED,
        KO = 255
    } TARGET_STATUS;

    Watchdog(pid_t target);
    virtual ~Watchdog();

    virtual void            shutdown_target(std::ostream *p_out);
    virtual TARGET_STATUS   get_target_status(std::ostream *p_err, std::ostream *p_out, int * p_child_return_value);

protected:

    virtual void kill_target(std::ostream *p_out);

protected:
    pid_t   m_target;

};


class WatchdogHelper
{
    public:
        int start(WATCHDOG_ENTRY_FUNCTION fun, void *fun_data, std::ostream *output_stream = NULL, std::ostream *err_stream = NULL);

    protected:
        int  child_process(WATCHDOG_ENTRY_FUNCTION fun, void *fun_data, std::ostream *output_stream = NULL, std::ostream *err_stream = NULL);
        bool parent_process(signal_handler& signal, pid_t child_pid, std::ostream *err_stream, std::ostream * out_stream, int *p_child_return_value);

};

} // namespace SB

#endif //#if defined LINUX_TARGET || defined RPI_TARGET

#endif //#ifndef SB_LIB_WATCHDOG_H

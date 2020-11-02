#if defined  LINUX_TARGET || defined RPI_TARGET

#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "watchdog/watchdog.h"

namespace SB
{

signal_handler * signal_handler::p_handler = NULL;

Watchdog::Watchdog(pid_t target): m_target(target)
{

}

Watchdog::~Watchdog()
{

}

Watchdog::TARGET_STATUS Watchdog::get_target_status(std::ostream *p_err, std::ostream *p_out, int * p_child_return_value)
{
    Watchdog::TARGET_STATUS return_value = Watchdog::OK;
    int target_status;
    pid_t ret_wait;

    if (m_target == 0 || p_child_return_value == NULL)
    {
        /* will trigger program restart */
        return Watchdog::KO;
    }

    /* poll the status of the child process */
    ret_wait = waitpid(m_target, &target_status, WNOHANG);

    if (ret_wait == m_target)
    {
        /* target process terminated */
        if (WIFEXITED(target_status) == 0 || WEXITSTATUS(target_status) == Watchdog::KO)
        {
            /* target process terminated unexpectedly */
            *p_err << "Watchdog::get_target_status(): Process with pid [" << m_target << "] is unexpectedly terminated, return status [" << WEXITSTATUS(target_status) << "]" << std::endl;
            *p_child_return_value = WEXITSTATUS(target_status);
            /* will trigger program restart */
            return_value = Watchdog::KO;
        }
        else
        {
            /* target process terminated spontaneously not asking for restart */
            *p_out << "Watchdog::get_target_status(): Process with pid [" << m_target << "] is normally terminated, return status [" << WEXITSTATUS(target_status) << "]" << std::endl;
            *p_child_return_value = WEXITSTATUS(target_status);
            return_value = Watchdog::DELIBERATELY_TERMINATED;
        }
    }
    else if (ret_wait == -1)
    {
        /* an error occurred */
        *p_err << "Watchdog::get_target_status(): Process with pid [" << m_target << "] will be terminated." << std::endl;

        kill_target(p_out);
        sleep(1);

        /* will trigger program restart */
        *p_child_return_value = EXIT_FAILURE;
        return_value = Watchdog::KO;
    }

    return return_value;
}

void Watchdog::kill_target(std::ostream *p_out)
{
    if (m_target > 0)
    {
        /* kill the target brutally */
        kill(m_target, SIGKILL);

       *p_out << "Watchdog::kill_target(): Process with pid [" << m_target << "] terminated by \"SIGKILL\" signal." << std::endl;
        m_target = 0;
    }
}

void Watchdog::shutdown_target(std::ostream *output_stream)
{
    int target_status;
    pid_t ret_wait;
    int counter = 0;
    const int max_count = 10; /* 10 seconds */

    if (m_target > 0)
    {
        do
        {
            /* try with a soft kill */
            kill(m_target, SIGTERM);

            sleep(1);

            ret_wait = 0;
            /* poll the status of the child process */
            ret_wait = waitpid(m_target, &target_status, WNOHANG);

            if (ret_wait != m_target && WIFEXITED(target_status))
            {
                /* target process terminated without error */
                *output_stream << "Watchdog::shutdown_target(): Process with pid [" << m_target << "] is normally terminated" << std::endl;
                break;
            }

            counter++;
        }
        while((counter < max_count) && (ret_wait != m_target));

        if (counter >= max_count)
        {
            /* kill the target brutally */
            kill_target(output_stream);
        }
    }
}

int WatchdogHelper::child_process(WATCHDOG_ENTRY_FUNCTION fun, void *fun_data, std::ostream *output_stream, std::ostream *err_stream)
{
    return fun(fun_data, output_stream, err_stream);
}

bool WatchdogHelper::parent_process(signal_handler& signal, pid_t child_pid, std::ostream *err_stream, std::ostream *out_stream, int *p_child_return_value)
{
    bool return_value = false;
    Watchdog watchdog(child_pid);
    Watchdog::TARGET_STATUS target_status;

    do
    {
        /* the manager is running */
        sleep(5);
        target_status = watchdog.get_target_status(err_stream, out_stream, p_child_return_value);
    }
    while ((!signal.get_exit_signal()) && (target_status == Watchdog::OK));

    if (target_status == Watchdog::KO)
    {
        /* will trigger program restart */
        return_value = true;
    }
    else if (target_status == Watchdog::DELIBERATELY_TERMINATED)
    {
        /* program will die without error */
        return_value = false;
    }
    else if (signal.get_exit_signal())
    {
        (*out_stream) << "watchdog: parent termination requested" << std::endl;
        /* if the exit was requested */
        /* trigger the exit of the child */
        watchdog.shutdown_target(out_stream);

        /* program will die without error */
        return_value = false;
    }

    return return_value;
}

int WatchdogHelper::start(WATCHDOG_ENTRY_FUNCTION fun, void *fun_data, std::ostream *output_stream, std::ostream *err_stream)
{
    int return_value = EXIT_SUCCESS;
    std::ostream * p_out = output_stream != NULL ? output_stream : &std::cout;
    std::ostream * p_err = err_stream != NULL ? err_stream : &std::cerr;
    signal(SIGPIPE, SIG_IGN);

    try
    {
        pid_t pid;
        bool restart_program;
        bool got_exit_signal = false;

        /* this cycle restarts the program if needed */
        do
        {
            pid = 0;
            restart_program = false;
            (*p_out) << "watchdog: starting main function" << std::endl;

            /* Fork off the parent process */
            pid = fork();
            if (pid < 0)
            {
                (*p_err) << "watchdog: Error during \"fork\" call" << std::endl;
                return_value = EXIT_FAILURE;
                break;
            }

            if (pid == 0)
            {
                /***************** child or not daemonized process *****************/
                return_value = child_process(fun, fun_data, p_out, p_err);
            }
            else if (pid > 0)
            {
                /************************* parent process **************************/
                signal_handler signal;        

                /* Register signal handler to handle signal */
                signal.setup_signal_handlers();
                restart_program = parent_process(signal, pid, p_err, p_out, &return_value);
                got_exit_signal = signal.get_exit_signal(); 
            }
        }
        while ((restart_program == true) && (!got_exit_signal));
    }
    catch (signal_exception& e)
    {
        (*p_err) << "watchdog: signal exception: " << e.what() << std::endl;
        return_value = EXIT_FAILURE;
    }
    catch (...)
    {
        (*p_err) << "watchdog: generic exception" << std::endl;
        return_value = EXIT_FAILURE;
    }
    return return_value;

}


} // namespace SB

#endif // #if defined  LINUX_TARGET || defined RPI_TARGET


    
#include <string.h>

#include "stm32f1xx.h"

#include "panglos/debug.h"
#include "panglos/list.h"
#include "panglos/io.h"
#include "panglos/drivers/gpio.h"

#include "cli/src/cli.h"

#include "timer.h"
#include "cli.h"

using namespace panglos;

    /*
     *  CLI Commands
     */

static int match_name(CliCommand *cmd, void *arg)
{
    ASSERT(arg);
    const char *s = (const char*) arg;
    return strcmp(cmd->cmd, s) == 0;
}

static void print_help(CLI *cli, CliCommand *cmd)
{
    ASSERT(cli);
    ASSERT(cmd);
    cli_print(cli, "%s : %s%s", cmd->cmd, cmd->help, cli->eol);
}

static void print_help_nest(CLI *cli, CliCommand *cmd, int nest)
{
    for (; cmd; cmd = cmd->next)
    {
        cli_print(cli, "%*s", nest, "");
        print_help(cli, cmd);
        if (cmd->subcommand)
        {
            print_help_nest(cli, cmd->subcommand, nest + 1);
        }
    }
}

static void cmd_help(CLI *cli, CliCommand *)
{
    const char *s = cli_get_arg(cli, 0);

    if (s)
    {
        CliCommand *cmd = cli_find(& cli->head, match_name, (void*) s);
        if (!cmd)
        {
            cli_print(cli, "command not found%s", cli->eol);
            return;
        }

        print_help(cli, cmd);
        return;
    }

    cli_print(cli, "\n\r");
    print_help_nest(cli, cli->head, 0);
}

static void cmd_reset(CLI *, CliCommand *)
{
    NVIC_SystemReset();
}

static void cmd_show(CLI *cli, CliCommand *)
{
    cli_print(cli, "\n\r");
    cli_print(cli, "period=%lu%s", period_us, cli->eol);
}

static void cmd_phase(CLI *cli, CliCommand *)
{
    int value = 0;
    int triac = 0;
    int idx = 0;

    cli_print(cli, "\n\r");

    const char *s = cli_get_arg(cli, idx++);
    if (!s)
    {
        cli_print(cli, "expected <phase>%s", cli->eol);
        return;
    }

    if (!cli_parse_int(s, & value, 0))
    {
        cli_print(cli, "invalid phase '%s'%s", s, cli->eol);
        return;
    }

    s = cli_get_arg(cli, idx++);
    if (s)
    {
        cli_parse_int(s, & triac, 0);
    }
    
    TIM3_SetPulseWidth(value);
    cli_print(cli, "set phase to %u%s", value, cli->eol);
    if (triac)
    {
        TIM4_SetPulseWidth(triac);
        cli_print(cli, "set triac pulse width to %u%s", triac, cli->eol);
    }
}

static void cmd_led(CLI *cli, CliCommand *)
{
    int value = 0;
    int idx = 0;

    cli_print(cli, "\n\r");

    const char *s = cli_get_arg(cli, idx++);
    if (!s)
    {
        cli_print(cli, "expected <value>%s", cli->eol);
        return;
    }

    if (!strcmp(s, "flash"))
    {
        flash = true;
        return;
    }

    if (!cli_parse_int(s, & value, 0))
    {
        cli_print(cli, "invalid value '%s'%s", s, cli->eol);
        return;
    }

    ASSERT(led);
    flash = false;
    led->set(!value);
}

static CliCommand cli_commands[] = {
    { "reset",  cmd_reset,  "CPU Reset", 0, 0, 0 },
    { "help",   cmd_help,   "help <cmd>", 0, 0, 0 },
    { "show",   cmd_show,   "show system state", 0, 0, 0 },
    { "phase",  cmd_phase,  "set triac phase <phase> [triac_pulse_width]", 0, 0, 0 },
    { "led",    cmd_led,    "set led <0|1|flash>", 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0 },
};

static void add_cli_commands(CLI *cli)
{
    for (size_t i = 0; cli_commands[i].cmd; i++)
    {
        cli_insert(cli, & cli->head, & cli_commands[i]);
    }
}

void init_cli(CLI *cli, Out *out)
{
    static FmtOut fmt_out(out, 0);

    static CliOutput output = {
        .fprintf = FmtOut::xprintf,
        .ctx = & fmt_out,
    };

    cli->output = & output;
    cli->eol = "\r\n";
    cli->prompt = "> ";
    cli_init(cli, 64, 0);
    cli->echo = false;

    add_cli_commands(cli);
}

//  FIN


#include <string.h>

#include "panglos/debug.h"
#include "panglos/drivers/gpio.h"

#include "cli/src/cli.h"

#include "mains_control.h"

    /*
     *  CLI commands
     */

void cli_keypress(CLI *, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;
    struct PowerManager::Message msg = { .type = PowerManager::Message::M_KEY };
    pm->messages.push(msg);
}

void cli_mode(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "mode=%s%s", PowerManager::mode_name(pm->get_mode()), cli->eol);
        return;
    }

    const int code = rlut(PowerManager::mode_lut, s);

    struct PowerManager::Message msg = {
        .type = PowerManager::Message::M_MODE,
        .value = code,
    };
    pm->messages.push(msg);
}

void cli_pulse(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <period>%s", cmd->cmd, cli->eol);
        return;
    }

    int period = 0;
    const bool ok = cli_parse_int(s, & period, 0);
    if (!ok)
    {
        cli_print(cli, "error in period '%s'%s", s, cli->eol);
        return;
    }

    pm->set_pulse(period);
}

    /*
     *
     */

void cli_forward(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const char *args[CLI_MAX_ARGS+1] = { 0 };

    for (int idx = 0; idx <= CLI_MAX_ARGS; idx++)
    {
        const char *s = cli_get_arg(cli, idx);
        if (!s) break;
        args[idx] = s;
    }

    for (int idx = 0; args[idx]; idx++)
    {
        pm->forward(args[idx]);
        pm->forward(" ");
    }
    pm->forward("\r\n");
}

    /*
     *
     */

void cli_show(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const struct TemperatureControlConfig *tc = 0;
    TemperatureControl *t = pm->get_temp_control();
    if (t) tc = t->get_config();

    cli_print(cli, "power=%dW%s", pm->get_power(), cli->eol);
    cli_print(cli, "percent=%d%% %s", pm->get_percent(), cli->eol);
    cli_print(cli, "phase=%d%s", pm->get_phase(), cli->eol);
    cli_print(cli, "mode=%s%s", pm->mode_name(pm->get_mode()), cli->eol);
    cli_print(cli, "temp=%dC%s", pm->get_temperature(), cli->eol);
    cli_print(cli, "error=%s%s", pm->get_error_mode(), cli->eol);
    if (tc)
    {
        cli_print(cli, "fan_on=%d fan_off=%d alarm=%d state=%d%s", 
                tc->fan_on, tc->fan_off, tc->alarm, 
                tc->fan ? tc->fan->get() : -1,
                cli->eol);
    }
}

    /*
     *  General "command off|value" handling
     */

enum Control { C_PHASE, C_TEMP, C_POWER };

static void cli_set(CLI *cli, CliCommand *cmd, enum Control c)
{
    ASSERT(cmd);
    PowerManager *pm = (PowerManager*) cmd->ctx;
    ASSERT(pm);

    static const LUT _lut[] = {
        { "phase", C_PHASE, },
        { "temp", C_TEMP, },
        { "power", C_POWER, },
        { 0 },
    };
    const char *name = lut(_lut, c);

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <%s>%s", name, name, cli->eol);
        return;
    }

    bool on = true;
    int var = 0;

    if (!strcmp("off", s))
    {
        on = false;
    }
    else
    {
        const bool ok = cli_parse_int(s, & var, 0);
        if (!ok)
        {
            cli_print(cli, "error in %s '%s'%s", name, s, cli->eol);
            return;
        }
    }

    switch (c)
    {
        case C_PHASE : pm->sim_phase(on, var);       break;
        case C_TEMP  : pm->sim_temperature(on, var); break;
        case C_POWER : pm->set_simulation(on, var);  break;
        default      : ASSERT(0);
    }        
}

    /*
     *
     */

void cli_power(CLI *cli, CliCommand *cmd)
{
    return cli_set(cli, cmd, C_POWER);
}

void cli_phase(CLI *cli, CliCommand *cmd)
{
    return cli_set(cli, cmd, C_PHASE);
}

void cli_temp(CLI *cli, CliCommand *cmd)
{
    return cli_set(cli, cmd, C_TEMP);
}

    /*
     *
     */

CliCommand cli_cmds[] = {
    { "key", cli_keypress, "simulate key press", 0, 0 },
    { "mode", cli_mode, "mode <eco|on|off|base>", 0, 0 },
    { "pulse", cli_pulse, "pulse <period>", 0, 0 },
    { "x", cli_forward, "forwards commands to send the stm32", 0, 0 },
    { "show", cli_show, "show status", 0, 0 },
    { "power", cli_power, "power N|off # simulate power value", 0, 0 },
    { "phase", cli_phase, "phase N|off # directly set triac phase", 0, 0 },
    { "temp", cli_temp, "temp N|off # simulate temperature", 0, 0 },
    { 0 },
};

//  FIN

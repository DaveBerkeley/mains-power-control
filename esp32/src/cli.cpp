
#include <string.h>

#include "panglos/debug.h"

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

void cli_sim(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <off|power>%s", cmd->cmd, cli->eol);
        return;
    }

    if (!strcmp(s, "off"))
    {
        cli_print(cli, "sim off%s", cli->eol);
        pm->set_simulation(false);
        return;
    }

    int power = 0;
    const bool ok = cli_parse_int(s, & power, 0);
    if (!ok)
    {
        cli_print(cli, "error in power '%s'%s", s, cli->eol);
        return;
    }

    pm->set_simulation(true, power);
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

    cli_print(cli, "power=%dW%s", pm->get_power(), cli->eol);
    cli_print(cli, "percent=%d%% %s", pm->get_percent(), cli->eol);
    cli_print(cli, "phase=%d%s", pm->get_phase(), cli->eol);
    cli_print(cli, "mode=%s%s", pm->mode_name(pm->get_mode()), cli->eol);
    cli_print(cli, "temp=%dC%s", pm->get_temperature(), cli->eol);
}

    /*
     *
     */

void cli_phase(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    PowerManager *pm = (PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <phase>%s", cmd->cmd, cli->eol);
        return;
    }

    if (!strcmp("off", s))
    {
        pm->sim_phase(false, 0);
        return;
    }

    int phase = 0;
    const bool ok = cli_parse_int(s, & phase, 0);
    if (!ok)
    {
        cli_print(cli, "error in phase '%s'%s", s, cli->eol);
        return;
    }

    pm->sim_phase(true, phase);
}

    /*
     *
     */

CliCommand cli_cmds[] = {
    { "key", cli_keypress, "simulate key press", 0, 0 },
    { "mode", cli_mode, "mode <eco|on|off|base>", 0, 0 },
    { "sim", cli_sim, "sim <on|off|power>", 0, 0 },
    { "pulse", cli_pulse, "pulse <period>", 0, 0 },
    { "x", cli_forward, "forwards commands to send the stm32", 0, 0 },
    { "show", cli_show, "show status", 0, 0 },
    { "phase", cli_phase, "phase N|off # directly set triac phase", 0, 0 },
    { 0 },
};

//  FIN

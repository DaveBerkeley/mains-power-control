
#include <string.h>

#include "panglos/debug.h"
#include "panglos/mqtt.h"

#include "mqtt.h" // for test MqttEvent definition

namespace panglos {

int MqttClient::publish(const char *topic, const char *data, int len, int qos, int retain)
{
    IGNORE(topic);
    IGNORE(data);
    IGNORE(len);
    IGNORE(qos);
    IGNORE(retain);
    ASSERT(0);
    return 0;
}

void MqttClient::add(panglos::MqttSub *sub)
{
    PO_DEBUG("%p", sub);
    subs.push(sub, 0);
}

void MqttClient::on_data(MqttEvent *event)
{
    // iterate through subscriptions
    for (MqttSub *sub = subs.head; sub; sub = sub->next)
    {
        ASSERT(sub->handler);
        if (strcmp(sub->topic, event->topic))
        {
            continue;
        }
        PO_DEBUG("topic match %s", event->topic);
        sub->handler(sub->arg, event->data, event->len);
    }
}

}   //  namespace panglos

//  FIN

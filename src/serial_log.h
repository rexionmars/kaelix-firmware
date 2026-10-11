#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include <cstdarg>
#include <cstdio>

namespace kaelix::logging {

// Relógio monotônico desde o boot, sem sincronização entre dispositivos.
// Usar apenas nas tarefas; nunca chamar de uma ISR.
inline void log(const char* device, const char* level, const char* event,
                const char* format, ...) {
    const uint64_t ms = static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
    char message[384];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    Serial.printf("[+%02llu:%02u:%02u.%03u][%s][%s][%s] %s\n",
        static_cast<unsigned long long>(ms / 3600000ULL),
        static_cast<unsigned>((ms / 60000ULL) % 60ULL),
        static_cast<unsigned>((ms / 1000ULL) % 60ULL),
        static_cast<unsigned>(ms % 1000ULL), device, level, event, message);
}

} // namespace kaelix::logging

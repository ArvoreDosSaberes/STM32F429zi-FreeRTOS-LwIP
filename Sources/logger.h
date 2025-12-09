#ifndef LOGGER_H
#define LOGGER_H

/**
 * @brief Inicializa os recursos do logger (mutex para sincronizar printf entre tasks).
 */
void loggerInit(void);

/**
 * @brief Versão thread-safe de printf usando mutex do FreeRTOS.
 *
 * Pode ser usada em qualquer tarefa. Antes do scheduler iniciar, o logger
 * apenas delega para vprintf sem usar mutex.
 */
void loggerPrint(const char *fmt, ...);

#endif /* LOGGER_H */

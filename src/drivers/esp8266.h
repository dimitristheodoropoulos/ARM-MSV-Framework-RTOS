#ifndef ESP8266_H
#define ESP8266_H

#define ESP_OK             0
#define ESP_ERR_TIMEOUT   -1
#define ESP_ERR_RESPONSE  -2

/* Αρχικοποίηση με έλεγχο αν το module απαντάει */
int esp8266_init(void);

/* Αποστολή εντολής και αναμονή για συγκεκριμένη απάντηση (π.χ. "OK") */
int esp8266_send_command(const char *cmd, const char *expected_resp, unsigned int timeout_ms);

#endif
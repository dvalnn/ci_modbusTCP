#include "applicationLayer/dataUnit.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

/**
 * @brief Create a new modbusPDU instance
 *
 * @param fCode function code
 * @param data  byte array
 * @param dataLen length of data
 * @return modbusPDU*
 */
modbusPDU* newModbusPDU(uint8_t fCode, uint8_t* data, uint8_t dataLen) {
    modbusPDU* pdu = malloc(sizeof(modbusPDU));
    if (pdu == NULL) {
        MALLOC_ERR;
        return NULL;
    }

    pdu->fCode = fCode;
    pdu->dataLen = dataLen;
    pdu->data = malloc(dataLen);
    if (pdu->data == NULL) {
        MALLOC_ERR;
        free(pdu);
        return NULL;
    }

    memcpy(pdu->data, data, dataLen);
    return pdu;
}

/**
 * @brief free a modbusPDU instance from memory along with its data
 *
 * @param pdu modbusPDU instance
 */
void freeModbusPDU(modbusPDU* pdu) {
    if (pdu == NULL) {
        return;
    }

    if (pdu->data != NULL) {
        free(pdu->data);
    }

    free(pdu);
}

/**
 * @brief serialize a modbusPDU instance into a byte array.
 *        The returned byte array must be freed by the caller.
 *
 * @param pdu modbusPDU instance
 * @param retArray pointer to the returned byte array
 * @return int length of the returned byte array
 */
int serializeModbusPDU(modbusPDU* pdu, uint8_t* retArray) {
    uint8_t* buffer = malloc(1 + pdu->dataLen);
    if (buffer == NULL) {
        MALLOC_ERR;
        return -1;
    }

    buffer[0] = pdu->fCode;
    memcpy(buffer + 1, pdu->data, pdu->dataLen);
    retArray = buffer;

    return 1 + pdu->dataLen;
}

#undef MALLOC_ERR
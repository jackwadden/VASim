/**
 * @file
 */

#include "errors.h"

//
const char * getErrorString(vasim_err_t error) {
    
    switch(error) {

    case E_SUCCESS :
        return "E_SUCCESS";
        break;
        
    case E_FILE_OPEN :
        return "E_FILE_OPEN";
        break;
        
    case E_ELEMENT_NOT_FOUND :
        return "E_ELEMENT_NOT_FOUND";
        break;

    case E_ELEMENT_NOT_SUPPORTED :
        return "E_ELEMENT_NOT_SUPPORTED";
        break;
        
    case E_MALFORMED_AUTOMATA :
        return "E_MALFORMED_AUTOMATA";
        break;
        
    default:
        return "E_UNKNOWN";
        
    }
}

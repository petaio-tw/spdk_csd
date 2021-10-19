/**
 * @file cs.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CS_H
#define CS_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 

/************************************************************/
/*                                                          */
/* MACRO FUNCTION DECLARATIONS                              */          
/* {MACRO functions defines for other components reference.}*/
/*                                                          */ 
/************************************************************/

/************************************************************/
/*                                                          */
/* DATA TYPE DECLARATIONS                                   */ 
/* {DATA TYPE defines for other components reference.}      */
/*                                                          */
/************************************************************/
typedef enum {
	CS_SUCCESS 			= 0,
	CS_NOT_ENOUGH_MEMORY,
	CS_OUT_OF_RESOURCES
} CS_STATUS;

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName);
CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer);
/* End of CS_H */
#endif

/**************************************************************************
 *                              clipboard.h                               *
 **************************************************************************                                                                   
 * Declaration of functions and structures that consist of an API for the *
 * app to interact with a remote synchronized clipboard					  *
 *************************************************************************/
#include <sys/types.h>

/******************************************************************************
* clipboard_connect - creates a local socket to communicate with the clipboard*
*                                        			                          *
* Receives: char *clipboard_dir - directory of the local clipboard            *   
* Returns: int sock_fd - the socket descriptor		                          *
******************************************************************************/
int clipboard_connect(char *clipboard_dir);

/******************************************************************************
* clipboard_copy -  copies the data pointed by buf to a region on the local   *
*  					clipboard	       										  *
*                                        			                          *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

/******************************************************************************
* clipboard_paste - copies from the system to the application the data in a   *
*					certain region											  *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

/******************************************************************************
* clipboard_wait -   waits for a change on a certain region                   *
*                                        			                          *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);

/******************************************************************************
* clipboard_close - closes the connection to the clipboard socket			  *
*                                        			                          *
* Receives: int clipboard_id - descriptor of the socket of the local clipboard*   
* Returns: void                  											  *
******************************************************************************/
void clipboard_close(int clipboard_id);

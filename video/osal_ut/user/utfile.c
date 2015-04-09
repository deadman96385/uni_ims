/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

 /* this should be all the user needs */
#include "osal_ut.h"

#define OSALUT_FILE_TEST_BUF_SZ         32

/*
 * ========= do_test_file() ========
 * Gen unit test vectors for each OSAL file function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_file(
    void)
{
    OSAL_FileId fid;
    char        fileName[] = "osalUt_fileTest";
    char        writeBuff[OSALUT_FILE_TEST_BUF_SZ];
    char        readBuff[OSALUT_FILE_TEST_BUF_SZ];
    vint        size, fileSize;
    vint        offset;

    OSAL_logMsg("File Unit Test Starting...\n");
    /* reset this before every test */
    osal_utErrorsFound = 0;

    /*
    * File test 1:
    * To test OSAL_fileOpen() with OSAL_FILE_O_CREATE flag, OSAL_fileClose()
    * and OSAL_fileExists().
    * In the beginning, the file "osalUt_fileTest" doesn't exist examined by
    * OSAL_fileExists(). After OSAL_fileOpen() with OSAL_FILE_O_CREATE flag,
    * the file exist and then close the file.
    */
    OSAL_logMsg("\nFile test 1\n");
    if (OSAL_TRUE == OSAL_fileExists(fileName)) {
        prError("It's not expecting that the file exists. \n");
    }
    else {
        if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, OSAL_FILE_O_CREATE,
                00755)) {
            prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_CREATE. \n");
        }
        else {
            /* After creating the file, to check if the file exists. */
            if (OSAL_TRUE != OSAL_fileExists(fileName)) {
                prError("It's not expecting that the file is not exists. \n");
            }

            if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
                prError("Failed to OSAL_fileClose(). \n");
            }
        }
    }

    /*
    * File test 2:
    * To test OSAL_fileOpen() with OSAL_FILE_O_WRONLY flag and
    * OSAL_fileWrite().
    * The file can't be read if it be opened with OSAL_FILE_O_WRONLY flag.
    * To check the result of OSAL_fileWrite() in next case.
    */
    OSAL_logMsg("\nFile test 2\n");
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, OSAL_FILE_O_WRONLY,
            0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_WRONLY. \n");
    }
    else {
        /* Writing the string "File test 2\n" to the file. */
        size = OSAL_snprintf(writeBuff, OSALUT_FILE_TEST_BUF_SZ, "%s\n",
                "File test 2");
        if (OSAL_SUCCESS != OSAL_fileWrite (&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
        }
        else {
            /*
            * The file can't be read because it be opened with
            * OSAL_FILE_O_WRONLY flag.
            */
            if (OSAL_SUCCESS == OSAL_fileRead(&fid, readBuff, &size)) {
                prError("It's not expecting that the file can be read. \n");
            }
        }

        if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
            prError("Failed to OSAL_fileClose(). \n");
        }
    }

    /*
    * File test 3:
    * To test OSAL_fileOpen() with OSAL_FILE_O_RDONLY flag, OSAL_fileGetSize()
    * and OSAL_fileRead().
    * Geting the file size and then to check if the size value is the same as
    * the writing string length. Reading the file contents and then compare
    * the reading buffer to the writing buffer. The file can't be write if it
    * be opened with OSAL_FILE_O_RDONLY flag.
    */
    OSAL_logMsg("\nFile test 3\n");
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, OSAL_FILE_O_RDONLY,
            0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDONLY. \n");
    }
    else {
        if (OSAL_SUCCESS != OSAL_fileGetSize(&fid, &size)) {
            prError("Failed to OSAL_fileGetSize(). \n");
        }
        else {
            if (size != OSAL_strlen(writeBuff)) {
                prError("The file size get by OSAL_fileGetSize() isn't "
                        "expecting. \n");
            }
        }
        if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
            prError("Failed to OSAL_fileRead(). \n");
        }
        else {
            if (0 != OSAL_strncmp(writeBuff, readBuff, size)) {
                prError("The contents of reading is not the same as "
                        "writing. \n");
            }

            size = OSAL_snprintf(writeBuff, OSALUT_FILE_TEST_BUF_SZ, "%s\n",
                    "File test 3");
            if (OSAL_SUCCESS == OSAL_fileWrite (&fid, writeBuff, &size)) {
                prError("It's not expecting that the file can be write. \n");
            }
        }

        if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
            prError("Failed to OSAL_fileClose(). \n");
        }
    }

    /*
    * File test 4:
    * To test OSAL_fileOpen() with (OSAL_FILE_O_APPEND | OSAL_FILE_O_RDWR)
    * flag and  OSAL_fileSeek() with OSAL_FILE_SEEK_CUR.
    * Appending a string to the end of file and then seek from current
    * position backword plus the length of writing string. To check if the
    * string is appened at the end of file. To read from the current location
    * and then compare the content to the appending string and the file
    * size should larger than the length of pending string.
    */
    OSAL_logMsg("\nFile test 4\n");
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, 
                (OSAL_FileFlags)(OSAL_FILE_O_APPEND | OSAL_FILE_O_RDWR), 0)) {
        prError("Failed to OSAL_fileOpen() with (OSAL_FILE_O_APPEND | "
                "OSAL_FILE_O_RDWR). \n");
    }
    else {
        /* Appending the string "File test 4\n" to the file. */
        size = OSAL_snprintf(writeBuff, OSALUT_FILE_TEST_BUF_SZ, "%s\n",
                "File test 4");
        if (OSAL_SUCCESS != OSAL_fileWrite (&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
        }
        else {
            size = OSALUT_FILE_TEST_BUF_SZ;
            OSAL_memSet(readBuff, 0, OSALUT_FILE_TEST_BUF_SZ);
            if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
                prError("Failed to OSAL_fileRead(). \n");
            }
            /*
            * Because the file pointer is at the end of file, it can't read
            * anything from it.
            */
            if (size == 0) {
                /*
                * Repositioning the file descriptor fildes from current
                * position backward plus the length of writing string.
                */
                offset = 0 - OSAL_strlen(writeBuff);
                if (OSAL_SUCCESS != OSAL_fileSeek(&fid, &offset,
                        OSAL_FILE_SEEK_CUR)) {
                    prError("Failed to OSAL_fileSeek(). \n");
                }
                else {
                    /*
                    * Reading the file to check if the content is the same as
                    * the appending string.
                    */
                    size = OSALUT_FILE_TEST_BUF_SZ;
                    if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
                        prError("Failed to OSAL_fileRead(). \n");
                    }
                    else {
                        if (0 != OSAL_strncmp(writeBuff, readBuff, size)) {
                            prError("The contents of reading is not the same "
                                    "as writing. \n");
                        }
                        /* The file size should larger than the length of the
                        * appending string.
                        */
                        if (OSAL_SUCCESS != OSAL_fileGetSize(&fid, &fileSize)) {
                            prError("Failed to OSAL_fileGetSize(). \n");
                        }
                        else {
                            if (fileSize < OSAL_strlen(writeBuff)) {
                                prError("The file size get by OSAL_fileGetSize()"
                                        " isn't expecting. \n");
                            }
                        }
                    }
                }
            }
            else {
                prError("It's not expecting that the file pointer isn't at "
                        "the end of file.  \n");
            }
        }

        if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
            prError("Failed to OSAL_fileClose(). \n");
        }
    }

    /*
    * File test 5:
    * To test OSAL_fileOpen() with (OSAL_FILE_O_TRUNC |OSAL_FILE_O_RDWR) flag.
    * Getting file size to check if the file is be truncated to length 0.
    */
    OSAL_logMsg("\nFile test 5\n");
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, 
                (OSAL_FileFlags)(OSAL_FILE_O_TRUNC | OSAL_FILE_O_RDWR), 0)) {
        prError("Failed to OSAL_fileOpen() with (OSAL_FILE_O_TRUNC | "
                "OSAL_FILE_O_RDWR). \n");
    }
    else {
        if (OSAL_SUCCESS != OSAL_fileGetSize(&fid, &size)) {
            prError("Failed to OSAL_fileGetSize(). \n");
        }
        else {
            if (0 != size) {
                prError("The file size get by OSAL_fileGetSize() isn't "
                        "expecting. \n");
            }
        }

        if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
            prError("Failed to OSAL_fileClose(). \n");
        }
    }

    /*
    * File test 6:
    * To test OSAL_fileSeek() with OSAL_FILE_SEEK_SET and OSAL_FILE_SEEK_END.
    */
    OSAL_logMsg("\nFile test 6\n");
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, fileName, OSAL_FILE_O_RDWR,
            0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
    }
    else {
        /* Writing the string "File test 6\n" to the file. */
        size = OSAL_snprintf(writeBuff, OSALUT_FILE_TEST_BUF_SZ, "%s\n",
                "File test 6");
        if (OSAL_SUCCESS != OSAL_fileWrite (&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
        }
        else {
            /*
              * Repositioning the file descriptor fildes from the start of
              * file forward plus 5 bytes.
              */
            offset = 5;
            if (OSAL_SUCCESS != OSAL_fileSeek(&fid, &offset,
                    OSAL_FILE_SEEK_SET)) {
                prError("Failed to OSAL_fileSeek(). \n");
            }
            else {
                OSAL_memSet(readBuff, 0, OSALUT_FILE_TEST_BUF_SZ);
                size = 4;
                if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
                    prError("Failed to OSAL_fileRead(). \n");
                }
                else {
                    if (0 != OSAL_strncmp("test", readBuff,
                            OSAL_strlen("test"))) {
                        prError("OSAL_fileSeek() with  OSAL_FILE_SEEK_SET "
                                "didn't success as expected. \n");
                    }
                }
            }

            /*
             * Repositioning the file descriptor fildes from the end of
             * file backward plus 2 bytes.
             */
            offset = -2;
            if (OSAL_SUCCESS != OSAL_fileSeek(&fid, &offset,
                    OSAL_FILE_SEEK_END)) {
                prError("Failed to OSAL_fileSeek(). \n");
            }
            else {
                OSAL_memSet(readBuff, 0, OSALUT_FILE_TEST_BUF_SZ);
                size = 1;
                if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
                    prError("Failed to OSAL_fileRead(). \n");
                }
                else {
                    if (0 != OSAL_strncmp("6", readBuff, OSAL_strlen("6"))) {
                        prError("OSAL_fileSeek() with  OSAL_FILE_SEEK_END "
                                "didn't success as expected. \n");
                    }
                }
            }
        }

        if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
            prError("Failed to OSAL_fileClose(). \n");
        }
    }

    /*
    * File test 7:
    * error case: to test OSAL_fileOpen() with invalid argument.
    */
    OSAL_logMsg("\nFile test 7\n");
    if (OSAL_FAIL != OSAL_fileOpen(NULL, fileName, OSAL_FILE_O_RDWR, 0)) {
        prError("OSAL_fileOpen()  didn't fail as expected. \n");
    }
    if (OSAL_FAIL != OSAL_fileOpen(&fid, NULL, OSAL_FILE_O_RDWR, 0)) {
        prError("OSAL_fileOpen()  didn't fail as expected. \n");
    }

    /* Delete the file */
    OSAL_sysExecute("rm", fileName);

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("File Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("File Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}


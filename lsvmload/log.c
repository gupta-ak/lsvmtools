/*
**==============================================================================
**
** LSVMTools 
** 
** MIT License
** 
** Copyright (c) Microsoft Corporation. All rights reserved.
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in 
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE
**
**==============================================================================
*/
#include "config.h"
#include <lsvmutils/alloc.h>
#include <lsvmutils/efifile.h>
#include "log.h"
#include "paths.h"
#include "console.h"
#include "globals.h"
#include <time.h>

static LogLevel _logLevel = INFO;
static time_t first = (time_t) 0;

void SetLogLevel(
    LogLevel logLevel)
{
    _logLevel = logLevel;
}

LogLevel GetLogLevel(void)
{
    return _logLevel;
}

EFI_STATUS TruncLog(void)
{
    return DeleteFile(globals.imageHandle, globals.lsvmlogPath);
}

static EFI_STATUS _WriteLog(const char* message)
{
    EFI_STATUS status = EFI_SUCCESS;
    static EFIFile* s_file = NULL;
    UINTN size;
    BOOLEAN enableIOHooks;

    /* Disable I/O hooks during this write */
    enableIOHooks = globals.enableIOHooks;
    globals.enableIOHooks = FALSE;

    /* Check parameters */
    if (!message)
    {
        status = EFI_INVALID_PARAMETER;
        goto done;
    }

    /* Save the size of the string */
    size = Strlen(message);

    /* Open the file */
    if (!s_file)
    {
        if (!(s_file = OpenFile(
            globals.imageHandle,
            globals.lsvmlogPath,
            EFI_FILE_MODE_WRITE | 
            EFI_FILE_MODE_READ | 
            EFI_FILE_MODE_CREATE,
            TRUE)) != EFI_SUCCESS)
        {
            goto done;
        }
    }

    /* Write the message to the file */
    if ((status = WriteFileN(s_file, message, size)) != EFI_SUCCESS)
    {
        goto done;
    }

    if ((status = FlushFile(s_file)) != EFI_SUCCESS)
    {
        goto done;
    }

done:

    /* Restore I/O hooks original setting */
    globals.enableIOHooks = enableIOHooks;

    return status;
}

const char* __logLevelStrings[] =
{
    "FATAL",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG"
};

static UINTN _numLogLevelStrings = 
    sizeof(__logLevelStrings) / sizeof(__logLevelStrings[0]);

int StrToLogLevel(
    const char* str, 
    LogLevel* logLevel)
{
    UINTN i;

    for (i = 0; i < _numLogLevelStrings; i++)
    {
        if (Strcmp(__logLevelStrings[i], str) == 0)
        {
            if (logLevel)
                *logLevel = (LogLevel)i;

            return 0;
        }
    }

    return -1;
}

EFI_STATUS PutLog(
    LogLevel logLevel,
    const CHAR16* format, 
    ...)
{
    EFI_STATUS status = EFI_SUCCESS;
    va_list ap;
    CHAR16* wcs = NULL;
    char* str = NULL;
    const char *logLevelStr = LogLevelToStr(logLevel);
    char num[32];
    char* numStart = &num[sizeof(num)/sizeof(num[0])];

    /* Check log level */
    if (logLevel > _logLevel)
    {
        goto done;
    }

    /* set time for first log entry */
    if (first == 0)
       first = time(NULL);

    /* Format the string */
    {
        va_start(ap, format);

        if (!(wcs = VPoolPrint((CHAR16*)format, ap)))
        {
            va_end(ap);
            status = EFI_OUT_OF_RESOURCES;
            goto done;
        }

        va_end(ap);
    }

    /* Get the time stamp to the log and convert to string. */
    {
	time_t t = time(NULL) - first;

	/* Format is "[NUM]: ". Work our way backwards. */
	*--numStart = '\0';
	*--numStart = ' ';
	*--numStart = ':';
	*--numStart = ']';
	do {
		*--numStart = '0' + (t % 10);
		t /= 10;
	}
	while (t > 0);
	*--numStart = '[';
    }

    /* Convert the string to single-character (ignore special characters) */
    {
        /* Log format is "LOGLEVEL: [TIMESTAMP]: MSG" */
        UINTN levelLen = Strlen(logLevelStr);
        UINTN numLen = Strlen(numStart);
        UINTN msgLen = StrLen(wcs);
        UINTN i;

        /* +2 for the ": " after LOGLEVEL and +2 for the ending "\n\0" */
        if (!(str = Malloc(sizeof(char) * (levelLen + numLen + msgLen + 4))))
        {
            status = EFI_OUT_OF_RESOURCES;
            goto done;
        }

        /* Copy the log level first. */
        Memcpy(str, logLevelStr, levelLen);
        str[levelLen] = ':';
        str[levelLen+1] = ' ';

        /* Now copy the time stamp. */
        Memcpy(&str[levelLen+2], numStart, numLen);

        /* Now copy the message string. */
        for (i = 0; i < msgLen; i++)
            str[levelLen + 2 + numLen + i] = (char)wcs[i];

        str[levelLen + 2 + numLen + msgLen] = '\n';
        str[levelLen + 2 + numLen + msgLen + 1] = '\0';
    }

    /* Write the string to the log */
    if ((status = _WriteLog(str)) != EFI_SUCCESS)
    {
        goto done;
    }

#if 0
    if (logLevel == ERROR || logLevel == FATAL)
    {
        Print(L"%a: %a", Str(logLevelStr), Str(str));
        Wait();
    }
#endif

done:

    if (str)
        Free(str);

    if (wcs)
        Free(wcs);

    return status;
}

/*********************************************************************************
 * Name        : ISTestTask                                                      *
 * Platform    : Windows 7 (Win32 API)                                           *
 * Developer   : Kuzmenko_AN                                                     *
 * Language    : cpp                                                             *
 * Description : Задан набор бинарных файлов. Разработать на C++ компонент,      * 
 *               который фиксирует начальное состояние этих файлов и позволяет   *
 *               обнаружить факт любых изменений в их содержимом                 *             
 *********************************************************************************/
// ISTestTask.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <tchar.h>
#include <sstream>

// constants
#define _BUFFER_SIZE_ 100
#define _FILE_INFO_ _T("digest.disi")
#define _SLEEP_TIME_  15000 // 

std::map<std::wstring, UINT32> digestSignatureMap;

void printLastError( LPCTSTR );                                                  // print  last error message
UINT32 getFileDigestSignature( LPWIN32_FIND_DATA );                              // set file signature and write up at temp file 
int writeFileSignatureAtFile( HANDLE , LPWIN32_FIND_DATA );                      // write up file information at temp file
void initDigestSignatureMap( HANDLE , HANDLE , LPWIN32_FIND_DATA , LPCTSTR);     // list a files and subdirectories
int checkFilesDigestSignature( HANDLE , LPWIN32_FIND_DATA , LPCTSTR );           // signature checker

int _tmain(int argc, _TCHAR* argv[])
{
    // variables for filesystem routine operations
    HANDLE hndlTmpFile = INVALID_HANDLE_VALUE;
    HANDLE hndlFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA win32FileInfo;

    // set digest file for information about files at directory and subdirectories with their fletcher's checksum
    hndlTmpFile = CreateFile( _FILE_INFO_ , FILE_APPEND_DATA , 0 , NULL , CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL , NULL );
    if( hndlTmpFile == INVALID_HANDLE_VALUE ){
        printLastError( _T("Tempory file was not created, fault!") );
        FindClose( hndlFile );
        CloseHandle( hndlTmpFile );
        std::wcin.get();
	    return 0;  
    }
    
    std::wcout << L"This program calculates the signature files in a directory and subdirectories in which it is located." << std::endl;
    std::wcout << L"The program checks the signature every " << _SLEEP_TIME_ 
                  << "  seconds, if the signatures do not match, it displays a list of files and the number of iteration." << std::endl; 

    std::wcout << std::endl << std::endl;

    // init catalog and files
    initDigestSignatureMap( hndlFile , hndlTmpFile , &win32FileInfo , L"*.*" );

    // check sigature map
    unsigned long iterate(0);
    do{
        std::wcout << L"Current iterate is " << ++iterate << std::endl;
        if( 1 == checkFilesDigestSignature( hndlFile , &win32FileInfo , L"*.*" ) ){
            std::wcout << "Files was not changed : all digest is valid" << std::endl;
        }
        Sleep(_SLEEP_TIME_);
        std::wcout << std::endl;
    }while(iterate <= MAXLONG32 );
    
    std::wcout << L"May Be long is not too kong?" << std::endl;

    std::wcin.get();

    FindClose( hndlFile );
    CloseHandle( hndlTmpFile );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////

/*********************************************************************************
 * Name        : printLastErorr                                                  *
 * Developer   : Kuzmenko_AN                                                     *
 * Description : print information about last error                              *
 * @param      : POINTER ON MESSAGE ( LPCTSTR )                                  *
 * @return     : VOID                                                            *
 *********************************************************************************/
void printLastError( LPCTSTR message ){
    std::wcout << "\t" << message << std::endl;
    std::wcout << "\t" << GetLastError() << std::endl;
}

/*********************************************************************************
 * Name        : writeFileSignature                                              *
 * Developer   : Kuzmenko_AN                                                     *
 * Description : write up information about finding files at temp                *
 * @param      : TEMP FILE HANDLE           ( HANDLE )                           *
 * @param      : POINTER TO WIN32_FIND_DATA ( LPWIN32_FIND_DATA )                *
 * @return     : -1 invalid parameters or 0 - success                            *
 *********************************************************************************/
int writeFileSignatureAtFile( HANDLE tmpHndl , LPWIN32_FIND_DATA lpwin32FD ){
    DWORD  dwWrittenBytes , dwReadBytes;
    UINT32 u32FileDigestSignature;
    TCHAR  digSigBiffer[_BUFFER_SIZE_] = L"<FILE> : ";

    if( tmpHndl == INVALID_HANDLE_VALUE || lpwin32FD == nullptr ){
        return -1;
    }

    // get a digest for file and create a record into temp file
    u32FileDigestSignature = getFileDigestSignature( lpwin32FD );
    
    // some conversion from int to wchar_t
    std::wstringstream wss;
    std::wstring wcs;
    wss << u32FileDigestSignature;
    wss >> wcs;
    LPCTSTR result = wcs.c_str();
    
    //std::wcout << u32FileDigestSignature << std::endl;

    digestSignatureMap.insert(std::pair<std::wstring , UINT32>(lpwin32FD->cFileName ,u32FileDigestSignature));
     
    wcsncat( digSigBiffer , result  , sizeof(u32FileDigestSignature)*sizeof(UINT32) );
    wcsncat( digSigBiffer , L" : " , wcslen(L" : ")*sizeof(wchar_t) );
    wcsncat( digSigBiffer , lpwin32FD->cFileName , wcslen(lpwin32FD->cFileName)*sizeof(wchar_t) );

    if( !WriteFile( tmpHndl , digSigBiffer , wcslen(digSigBiffer)*sizeof(wchar_t) , &dwWrittenBytes , NULL ) ){
        printLastError( _T("Cannot write at file") );
        return -1;
    }
    WriteFile( tmpHndl , L"\r\n" , sizeof(wchar_t) * 2 , &dwWrittenBytes , NULL );
    return 0;
}

/*********************************************************************************
 * Name        : getFileDigestSignature                                          *
 * Developer   : Kuzmenko_AN                                                     *
 * Description : get a digest signature for file with fletcher checksum method   *
 * @param      : POINTER TO WIN32_FIND_DATA WITH FILE INFO ( LPWIN32_FIND_DATA ) *
 * @return     : DIGEST SIGNATURE  ( UINT32 )                                    *
 *********************************************************************************/
UINT32 getFileDigestSignature( LPWIN32_FIND_DATA lpwin32FD ){
    DWORD nRead;
    HANDLE hndlFile;
    UINT32 fileSignature[ 1024 ];
    
    hndlFile = CreateFile( lpwin32FD->cFileName , GENERIC_READ , 0 , NULL , OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , NULL );
    if( FALSE == ReadFile( hndlFile , fileSignature , sizeof(fileSignature) , &nRead , NULL ) ){
        std::wcout << "Cannot read a file at getFileDigestSignature method" << std::endl;
        return 0x00000000;
    }

    UINT32 dateSignature = ( lpwin32FD->ftCreationTime.dwHighDateTime ^ lpwin32FD->ftCreationTime.dwLowDateTime ) | 
                            ( lpwin32FD->ftLastAccessTime.dwHighDateTime ^ lpwin32FD->ftLastAccessTime.dwLowDateTime ) |
                             ( lpwin32FD->ftLastWriteTime.dwHighDateTime ^ lpwin32FD->ftLastWriteTime.dwLowDateTime );

    ////signature calculation
    UINT32 sum1 = 1, sum2 = 0;
    for(unsigned int i = 0 ; i < 1024 ; ++i){
        dateSignature ^= fileSignature[i];
        sum1 = (sum1 + dateSignature) % 65521;
        sum2 = (sum1 + sum2) % 65521;
    }
    sum2 =  sum2 << 16 + sum1;
    dateSignature ^= sum2;

    CloseHandle( hndlFile );
    return dateSignature;
}

/*********************************************************************************
 * Name        : initDigestSignatureMap                                          *
 * Developer   : Kuzmenko_AN                                                     *
 * Description : init a digest signature for each file                           *
 * @param      : SEARCH HANDLE              ( HANDLE )                           *
 * @param      : TEMP FILE HANDLE           ( HANDLE )                           *
 * @param      : POINTER TO WIN32_FIND_DATA ( LPWIN32_FIND_DATA )                *
 * @param      : pattern for search         ( LPCTSTR )                          *
 * @return     : VOID                                                            *
 *********************************************************************************/
 void initDigestSignatureMap( HANDLE sHndl , HANDLE tmpHndl , LPWIN32_FIND_DATA lpwin32FD , LPCTSTR searchPattern ){
    static short unsigned int tabs = 0;     // tabs for formatting console output
    
    sHndl = FindFirstFile( searchPattern , lpwin32FD );
    do{
        if( wcscmp( lpwin32FD->cFileName , L"." ) == 0  || 
            wcscmp( lpwin32FD->cFileName , L".." ) == 0 || 
            wcscmp( lpwin32FD->cFileName , _FILE_INFO_ ) ==0   ) continue;

        if( lpwin32FD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
            //std::wcout << _T("<DIRECTORY> : ") << lpwin32FD->cFileName << std::endl;
            SetCurrentDirectory( lpwin32FD->cFileName ); 
            tabs = tabs + 5;
            initDigestSignatureMap( sHndl , tmpHndl , lpwin32FD , searchPattern );
            tabs = tabs - 5;
            SetCurrentDirectory( _T("..") );
        }
        else{
            //std::wcout << std::setw(tabs) << " " << _T("<FILE> : ") << lpwin32FD->cFileName << std::endl;
            if( -1 == writeFileSignatureAtFile( tmpHndl , lpwin32FD ) ){
                printLastError( _T("Writing Operations is fault") );
                return;
            }
        }
    }while( FindNextFile( sHndl , lpwin32FD ) );
 }

/*********************************************************************************
 * Name        : checkFilesDigestSignature                                       *
 * Developer   : Kuzmenko_AN                                                     *
 * Description : check files signature and print list of bad guys                *
 * @param      : SEARCH HANDLE              ( HANDLE )                           *
 * @param      : POINTER TO WIN32_FIND_DATA ( LPWIN32_FIND_DATA )                *
 * @param      : pattern for search         ( LPCTSTR )                          *
 * @return     : INT                                                             *
 *********************************************************************************/
int checkFilesDigestSignature( HANDLE hndl , LPWIN32_FIND_DATA lpwin32FD , LPCTSTR pattern ){
    WCHAR dirName[260]; // buffer for directory name
    int flg = 1;

    hndl = FindFirstFile( pattern , lpwin32FD );
    GetCurrentDirectory( MAX_PATH , dirName );
    do{
        if( wcscmp( lpwin32FD->cFileName , L"." ) == 0  || 
            wcscmp( lpwin32FD->cFileName , L".." ) == 0 || 
            wcscmp( lpwin32FD->cFileName , _FILE_INFO_ ) ==0   ) continue;

        if( lpwin32FD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
            wcsncat( dirName , lpwin32FD->cFileName , sizeof(lpwin32FD->cFileName) );
            SetCurrentDirectory( lpwin32FD->cFileName ); 
            flg = checkFilesDigestSignature( hndl , lpwin32FD , pattern );
            SetCurrentDirectory( _T("..") );
        }
        else{
            UINT32 dsCurr = getFileDigestSignature( lpwin32FD );                    
            UINT32 dsInit = ( digestSignatureMap.find( lpwin32FD->cFileName ) != digestSignatureMap.end() ) 
                                                                ? digestSignatureMap.find( lpwin32FD->cFileName )->second : 0x00000000; 
            if( dsCurr !=  dsInit ){
                std::wcout << L"<DIR> : " << dirName <<L" : <FILE> : " << lpwin32FD->cFileName << " >>> ";
                std::wcout << "init DS : " << dsInit << " , current DS : " << dsCurr << std::endl; 
                flg = -1;
            } 
        }
    }while( FindNextFile( hndl , lpwin32FD ) );   
    return flg;
 }
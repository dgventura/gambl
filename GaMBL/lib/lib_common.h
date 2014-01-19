
// Common headers used by library

// Game_Music_Box 0.5.2. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef LIB_COMMON_H
#define LIB_COMMON_H

#ifndef TARGET_OS_IPHONE
    #include <Cocoa/Cocoa.h>
#endif

#include "blargg_common.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <iostream>
#include <vector>
#include <memory>

/*inline std::string& operator=(std::string & lhs, const std::string & rhs) {
    return lhs.assign(rhs);
}*/

class OpenFile
{
public:
    OpenFile( const char* pszPath, const char* pszMode )
    {
        m_pHandle = fopen( pszPath, pszMode );
        assert( m_pHandle );
        
        m_strPath.resize( strlen(pszPath) );
        mbstowcs( &m_strPath[0], pszPath, m_strPath.length() );
        std::wcout << "Opened new file: " << m_strPath << std::endl;
    }
    
    ~OpenFile()
    {
        fclose( m_pHandle );
        m_pHandle = NULL;
        
        std::wcout << "Closed file: " << m_strPath << std::endl;
    }
    
    const std::wstring& GetFilename()
    {
        return m_strPath;
    }
    
    FILE* m_pHandle;
        std::wstring m_strPath;
    
private:
    OpenFile();
    OpenFile( const OpenFile& rhs );

};

//RAD
class GaMBLFileHandle
{
public:
    GaMBLFileHandle()
    {
    }
    
    GaMBLFileHandle( const GaMBLFileHandle& rhs )
    {
        assert( rhs.IsOk() );
        
        std::wstring strPath;
        rhs.GetFilePath( strPath, true );
        OpenFileFromPath( strPath, "r" );
        
        assert( IsOk() );
    }
    
    GaMBLFileHandle( const std::wstring& strPath, const char* pszMode )
    {
        OpenFileFromPath( strPath, pszMode );
    }
    
    GaMBLFileHandle( const char* const pszPath, const char* pszMode )
    {
        OpenFileFromPath( pszPath, pszMode );
    }
    
    bool OpenFileFromPath( const std::wstring& strPath, const char* pszMode )
    {
        char szPath[PATH_MAX * sizeof(strPath[0])];
        const wchar_t* wcs = strPath.c_str();
        wcsrtombs( szPath, &wcs, sizeof(szPath), NULL );
     
        return OpenFileFromPath( szPath, pszMode );
    }
    
    bool OpenFileFromPath( const char* const pszPath, const char* pszMode )
    {
        CloseFile();
        
        m_pHandle = OpenFile( pszPath, pszMode );//fopen( pszPath, pszMode );
        assert( m_pHandle );
        
        m_nSeekPos = 0;
        
        return m_pHandle != NULL;
    }
    
    virtual ~GaMBLFileHandle()
    {
        CloseFile();
    }
    
    int GetDescriptor() const
    {
        assert( IsOk() );
        return fileno( m_pHandle->m_pHandle );
    }
    
    int GetFilePath( std::wstring& strPath, bool bResolveSymlinks ) const
    {
        assert( IsOk() );
        
        strPath = m_pHandle->m_strPath;
        
        if ( bResolveSymlinks )
        {
            char szTemp[PATH_MAX], szMbPath[PATH_MAX];
            wcstombs( szMbPath, strPath.c_str(), sizeof(szMbPath) );
            char* pszError = realpath( szMbPath, szTemp );
            assert( pszError );
            
            CFStringRef inPath = CFStringCreateWithCString( kCFAllocatorDefault, szMbPath, kCFStringEncodingUTF8 );
            CFStringRef resolvedPath = nil;
            CFURLRef	url = CFURLCreateWithFileSystemPath(NULL /*allocator*/, (CFStringRef)inPath, kCFURLPOSIXPathStyle, NO /*isDirectory*/);
            if (url != NULL) {
                FSRef fsRef;
                if (CFURLGetFSRef(url, &fsRef)) {
                    Boolean targetIsFolder, wasAliased;
                    if (FSResolveAliasFile (&fsRef, true /*resolveAliasChains*/, &targetIsFolder, &wasAliased) == noErr && wasAliased) {
                        CFURLRef resolvedurl = CFURLCreateFromFSRef(NULL /*allocator*/, &fsRef);
                        if (resolvedurl != NULL) {
                            resolvedPath = CFURLCopyFileSystemPath(resolvedurl, kCFURLPOSIXPathStyle);
                            CFRelease(resolvedurl);
                            
                            CFStringGetCString( resolvedPath, szMbPath, sizeof(szMbPath), kCFStringEncodingUTF8 );
                            
                            const int nPathLength = strlen( szMbPath );
                            assert( nPathLength );
                            strPath.resize( nPathLength + 1 );
                            mbstowcs( &strPath[0], szMbPath, strPath.length() );
                        }
                    }
                }
                CFRelease(url);

            }
            
            /*CFStringRef pathStr = CFStringCreateWithCString( kCFAllocatorDefault, szMbPath, kCFStringEncodingUTF8 );
            CFURLRef pathURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, pathStr, kCFURLPOSIXPathStyle, false );
            CFErrorRef errRef;
            CFDataRef dataRef = CFURLCreateBookmarkDataFromFile( kCFAllocatorDefault, pathURL, &errRef );
            pathStr = CFErrorCopyDescription( errRef );
            assert( dataRef );
            CFURLBookmarkResolutionOptions resOptions = 0;
            pathURL = CFURLCreateByResolvingBookmarkData( kCFAllocatorDefault, dataRef, resOptions, 0, NULL, NULL, NULL );
            assert( pathURL );
            pathStr = CFURLGetString( pathURL );*/
            
            
                    }

        return 0;
    }
    
    bool IsOk() const
    {
        if ( !m_pHandle )
        {
            printf( "File error: %s\n", strerror(errno) );
        }
        return m_pHandle != NULL;
    }
    
    long GetSize() const
    {
        assert( m_pHandle );
        
        fpos_t savePosition;
        fgetpos( m_pHandle->m_pHandle, &savePosition );
        
        fseek( m_pHandle->m_pHandle, 0L, SEEK_END );
        long nSize = ftell( m_pHandle->m_pHandle );
        
        fsetpos( m_pHandle->m_pHandle, &savePosition );
        
        return nSize;
    }
    
    bool Tell( SInt64* pnPos )
    {
        assert( m_pHandle );
        
        //*pnPos = ftell( m_pHandle->m_pHandle );
        *pnPos = m_nSeekPos;
        return true;
    }
    
    bool Seek( SInt64 nPos )
    {
        assert( m_pHandle );
        
        m_nSeekPos = nPos;
        fseek( m_pHandle->m_pHandle, nPos, SEEK_SET );
        return true;
    }
    
    long ReadBytes( void* p, long nTarget )
    {
        assert( m_pHandle );
        
        Seek( m_nSeekPos );
        long nBytesRead = fread( p, 1, nTarget, m_pHandle->m_pHandle );
        
        m_nSeekPos += nBytesRead;
        
        return nBytesRead;
    }
    
    GaMBLFileHandle& operator=( const GaMBLFileHandle& rhs )
    {
        m_pHandle = NULL;
        m_pHandle = rhs.m_pHandle;//fdopen( rhs.GetDescriptor(), "r" );
        m_nSeekPos = rhs.m_nSeekPos;
        
        assert( IsOk());
        
        return *this;
    }
    
        typedef std::shared_ptr< OpenFile > FilePointer;
    
private:
    void CloseFile()
    {
//        fclose( m_pHandle );
        m_pHandle = NULL;
        PurgeReferences();
    }
//    FILE* m_pHandle;
 
    SInt64 m_nSeekPos;
    FilePointer m_pHandle;
    static std::vector< FilePointer > m_vecpFiles;
    
    static FilePointer OpenFile( const char* pszPath, const char* pszMode )
    {
        std::wstring strTargetPath;
        strTargetPath.resize( strlen(pszPath) );
        mbstowcs( &strTargetPath[0], pszPath, strTargetPath.length() );
        
        for ( int i = 0; i < m_vecpFiles.size(); ++i )
        {
            if ( m_vecpFiles[i]->GetFilename() == strTargetPath )
                return m_vecpFiles[i];
        }
        
        FilePointer pNewElement = FilePointer( new class OpenFile(pszPath, pszMode) );
        m_vecpFiles.push_back( pNewElement );
        return m_vecpFiles.back();
    }
    
    static void PurgeReferences()
    {
        std::vector< FilePointer >::iterator it;
        for ( it = m_vecpFiles.begin(); it != m_vecpFiles.end(); ++it )
        {
            if ( (*it).use_count() == 1 )
            {
                m_vecpFiles.erase( it );
                --it;
            }
        }
    }

};

extern GaMBLFileHandle DummyHandle;

#include "debug_out.h"
#include "util.h"
#include "error_util.h"
#include "runtime_array.h"

using namespace std;

#endif


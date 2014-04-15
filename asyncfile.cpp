#include "asyncfile.h"

#include <QDebug>
#include <QThread>

static QString errorMessage(DWORD error)
{
    void *lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                  | FORMAT_MESSAGE_FROM_SYSTEM
                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  error,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (wchar_t *) &lpMsgBuf,
                  0, NULL );

    auto res = QString::fromWCharArray((wchar_t*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return res;
}

QString fileErrorToString(QFileDevice::FileError error)
{
    switch (error) {
    case QFileDevice::NoError: return AsyncFile::tr("NoError");
    case QFileDevice::ReadError: return AsyncFile::tr("ReadError");
    case QFileDevice::WriteError: return AsyncFile::tr("WriteError");
    case QFileDevice::FatalError: return AsyncFile::tr("FatalError");
    case QFileDevice::ResourceError: return AsyncFile::tr("ResourceError");
    case QFileDevice::OpenError: return AsyncFile::tr("OpenError");
    default:
        break;
    }
    Q_UNREACHABLE();
    return AsyncFile::tr("UnknownError");
}

AsyncFile::AsyncFile(QObject *parent) :
    QIODevice(parent),
    overlapped(MyOverlapped(this))
{
}

AsyncFile::AsyncFile(const QString &path, QObject *parent):
    QIODevice(parent),
    m_filePath(path),
    overlapped(MyOverlapped(this)),
    m_FileHandle(0),
    reading(false)
{

}

qint64 AsyncFile::bytesAvailable() const
{
    return m_readBuffer.size();
}

bool AsyncFile::open(QIODevice::OpenMode mode)
{
#ifdef Q_OS_WIN
    m_FileHandle = CreateFile(reinterpret_cast<wchar_t *>(QString(m_filePath).data()), // file to open
                              GENERIC_READ,           // open for reading
                              FILE_SHARE_READ,        // share for reading
                              NULL,                   // default security
                              OPEN_EXISTING,          // existing file only
                              FILE_FLAG_OVERLAPPED,   // overlapped operation
                              NULL);                  // no attr. template

    if (m_FileHandle == INVALID_HANDLE_VALUE) {
        setError(QFileDevice::OpenError, errorMessage(GetLastError()));
        return false;
    }

    if (!asyncRead(pos())) {
        setError(QFileDevice::OpenError, errorMessage(GetLastError()));
        return false;
    }
#endif

    return QIODevice::open(mode | QIODevice::Unbuffered);
}

qint64 AsyncFile::size() const
{
    if (!m_FileHandle)
        return 0;
    quint32 high, low;
    low = GetFileSize(m_FileHandle, (LPDWORD)&high);
    return (quint64(high) << 32) + low;
}

bool AsyncFile::waitForReadyRead(int msecs)
{
//    qDebug() << "waitForReadyRead";
    DWORD dwWaitOvpOprn = WaitForSingleObjectEx(overlapped.overlapped.hEvent, msecs, true);
//    qDebug() << "waitForReadyRead finished" << errorMessage(dwWaitOvpOprn);
    switch (dwWaitOvpOprn) {
    case WAIT_FAILED:
        return false;
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    }

//    if( m_bAborted )
//    {
//        return FALSE;
//    }

    return true;
}

qint64 AsyncFile::readData(char *data, qint64 maxlen)
{
    qint64 size = qMin((qint64)m_readBuffer.size(), maxlen);
    qDebug() << "readData" << maxlen << size;
    if (size == 0)
        return 0;
    qDebug() << pos() + maxlen << this->size();
//    Q_ASSERT(pos() + maxlen <= this->size());

    memcpy(data, m_readBuffer.data(), size);
    m_readBuffer.remove(0, size);
    Q_ASSERT(m_readBuffer.isEmpty());

    asyncRead(pos() + size);

    return size;
}

qint64 AsyncFile::writeData(const char *data, qint64 maxlen)
{
    return 0;
}

void AsyncFile::setError(QFileDevice::FileError error)
{
    if (m_error == error)
        return;
    m_error = error;
    setErrorString(::fileErrorToString(error));
}

void AsyncFile::setError(QFileDevice::FileError error, const QString &message)
{
    if (m_error == error)
        return;
    m_error = error;
    setErrorString(message);
}

void AsyncFile::readFinished(qint64 bytes)
{
    reading = false;
    m_readBuffer.append(QByteArray(overlapped.buffer, bytes));
//    memset(overlapped.buffer, 0, BUFFER_SIZE);

    emit readyRead();
//    QMetaObject::invokeMethod(this, "asyncRead", Qt::QueuedConnection);
    asyncRead(pos());
}

void AsyncFile::readCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped)
{
    qDebug() << "readCallback" << numberOfBytesTransfered;
    MyOverlapped *myOverlapped = reinterpret_cast<MyOverlapped *>(overlapped);
    myOverlapped->that->readFinished(numberOfBytesTransfered);
}

bool AsyncFile::asyncRead(qint64 pos)
{
    if (reading)
        return true;

    qint64 bytes = qMin(BUFFER_SIZE, MAX_BUFFER_SIZE - m_readBuffer.size());
    const qint64 offset = pos + m_readBuffer.size();
    bytes = qMin(bytes, size() - offset);
    qDebug() << "asyncRead" << "pos" << pos << "size" << size() << "bytes" << bytes;
    if (bytes <= 0)
        return true;
    Q_ASSERT(offset + bytes <= size());
    overlapped.overlapped.Offset = offset;

    bool ok = ReadFileEx(m_FileHandle,
                         overlapped.buffer,
                         bytes,
//                             &dwBytesRead,
                         &overlapped.overlapped,
                         &readCallback);
    if (!ok) {
        setError(QFileDevice::ReadError, errorMessage(GetLastError()) );
    }
    reading = ok;
    qDebug() << "asyncRead" << "finished" << ok;
    return ok;
}

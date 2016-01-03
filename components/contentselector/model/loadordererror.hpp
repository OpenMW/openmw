#ifndef LOADORDERERROR_HPP
#define LOADORDERERROR_HPP

#include <QString>

namespace ContentSelectorModel
{
    /// \brief Details of a suspected Load Order problem a plug-in will have. This is basically a POD.
    class LoadOrderError
    {
    public:
        enum ErrorCode
        {
            ErrorCode_None               = 0,
            ErrorCode_MissingDependency  = 1,
            ErrorCode_InactiveDependency = 2,
            ErrorCode_LoadOrder          = 3
        };

        inline LoadOrderError() : mErrorCode(ErrorCode_None) {}
        inline LoadOrderError(ErrorCode errorCode, QString fileName)
            : mErrorCode(errorCode), mFileName(fileName) {}
        inline ErrorCode errorCode() const { return mErrorCode; }
        inline QString fileName() const { return mFileName; }
        QString toolTip() const;

    private:
        ErrorCode mErrorCode;
        QString mFileName;
        static QString sErrorToolTips[ErrorCode_LoadOrder];
    };
}

#endif // LOADORDERERROR_HPP

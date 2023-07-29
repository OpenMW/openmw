#ifndef COMPONENTS_LOADINGLISTENER_ASYNCLISTENER_H
#define COMPONENTS_LOADINGLISTENER_ASYNCLISTENER_H

#include <mutex>
#include <optional>
#include <string>

#include "loadinglistener.hpp"

namespace Loading
{
    class AsyncListener : public Listener
    {
    public:
        AsyncListener(Listener& baseListener)
            : mBaseListener(baseListener)
        {
        }

        void setLabel(const std::string& label, bool important) override
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mLabelUpdate = label;
            mImportantLabel = important;
        }

        void setProgressRange(size_t range) override
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mRangeUpdate = range;
        }

        void setProgress(size_t value) override
        {
            std::lock_guard<std::mutex> guard(mMutex);
            mProgressUpdate = value;
        }

        void increaseProgress(size_t increase) override
        { /* not implemented */
        }

        void update()
        {
            std::lock_guard<std::mutex> guard(mMutex);
            if (mLabelUpdate)
                mBaseListener.setLabel(*mLabelUpdate, mImportantLabel);
            if (mRangeUpdate)
                mBaseListener.setProgressRange(*mRangeUpdate);
            if (mProgressUpdate)
                mBaseListener.setProgress(*mProgressUpdate);
            mLabelUpdate = std::nullopt;
            mRangeUpdate = std::nullopt;
            mProgressUpdate = std::nullopt;
        }

    private:
        Listener& mBaseListener;
        std::mutex mMutex;
        std::optional<std::string> mLabelUpdate;
        bool mImportantLabel = false;
        std::optional<size_t> mRangeUpdate;
        std::optional<size_t> mProgressUpdate;
    };
}

#endif // COMPONENTS_LOADINGLISTENER_ASYNCLISTENER_H

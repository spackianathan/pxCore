#include "pxTextureCacheObject.h"
#include "rtLog.h"
#include "pxUtil.h"
#include "pxContext.h"
#include "pxFileDownloader.h"
#include "pxScene2d.h"

extern pxContext context;

#include <map>
using namespace std;

typedef map<rtString, pxTextureRef> TextureMap;
TextureMap gCompleteTextureCache;

extern int gFileDownloadsPending;

void pxTextureCacheObject::onFileDownloadComplete(pxFileDownloadRequest* downloadRequest)
{
    if (downloadRequest == NULL)
    {
        return;
    }
    if (downloadRequest->getDownloadStatusCode() == 0 &&
            downloadRequest->getHttpStatusCode() == 200 &&
            downloadRequest->getDownloadedData() != NULL)
    {
        pxOffscreen imageOffscreen;
        if (pxLoadImage(downloadRequest->getDownloadedData(),
                downloadRequest->getDownloadedDataSize(),
                imageOffscreen) != RT_OK)
        {
            rtLogError("Image Decode Failed: %s", downloadRequest->getFileURL().cString());
            if (mParent != NULL)
            {
              mParent->onTextureReady(this, RT_FAIL);
            }
        }
        else
        {
            mTexture = context.createTexture(imageOffscreen);
            gCompleteTextureCache.insert(pair<rtString,pxTextureRef>(mURL.cString(),
                    mTexture));
            rtLogDebug("image %f, %f", mTexture->width(), mTexture->height());
            if (mParent != NULL)
            {
              mParent->onTextureReady(this, RT_OK);
            }
        }
    }
    else
    {
        rtLogWarn("Image Download Failed: %s Error: %s HTTP Status Code: %ld",
                downloadRequest->getFileURL().cString(),
                downloadRequest->getErrorString().cString(),
                downloadRequest->getHttpStatusCode());
        if (mParent != NULL)
        {
          mParent->onTextureReady(this, RT_FAIL);
        }
    }

}

rtError pxTextureCacheObject::url(rtString& s) const
{ 
  s = mURL; 
  return RT_OK;
}

rtError pxTextureCacheObject::setURL(const char* s)
{
  mURL = s;
  if (!s) 
    return RT_OK;
  loadImage(mURL);
  return RT_OK;
}

void pxTextureCacheObject::setParent(pxObject* parent)
{
    mParent = parent;
}

void pxTextureCacheObject::loadImage(rtString url)
{
  TextureMap::iterator it = gCompleteTextureCache.find(url);
  if (it != gCompleteTextureCache.end())
  {
    mTexture = it->second;
    if (mParent != NULL)
    {
      mParent->onTextureReady(this, RT_OK);
    }
  }
  else
  {
    rtLogWarn("Image texture cache miss");
    char* s = url.cString();
    const char *result = strstr(s, "http");
    int position = result - s;
    if (position == 0 && strlen(s) > 0)
    {
      pxFileDownloadRequest* downloadRequest = 
        new pxFileDownloadRequest(s, this);
      gFileDownloadsPending++;
      pxFileDownloader::getInstance()->addToDownloadQueue(downloadRequest);
    }
    else 
    {
      pxOffscreen imageOffscreen;
      if (pxLoadImage(s, imageOffscreen) != RT_OK)
        rtLogWarn("image load failed"); // TODO: why?
      else
      {
        mTexture = context.createTexture(imageOffscreen);
        gCompleteTextureCache.insert(pair<rtString,pxTextureRef>(s, mTexture));
        if (mParent != NULL)
        {
          mParent->onTextureReady(this, RT_OK);
        }
      }
    }
  }
}
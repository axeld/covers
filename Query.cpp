/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "Query.h"

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <HttpRequest.h>
#include <TranslationUtils.h>

#include <map>

#include <regex.h>
#include <stdio.h>


#define MAX_GROUPS 10
#define MAX_MATCHES 10

#define COUNTRY_GERMANY 3

#define TYPE_ORIGINAL "SCRM"
#define TYPE_LARGE "SCL"
#define TYPE_THUMB "THUMB"


enum ImageSize {
	kThumb,
	kLarge,
	kOriginal,
};


class DataListener : public BUrlProtocolListener {
public:
	DataListener()
	{
	}

	virtual ~DataListener()
	{
	}

	virtual void DataReceived(BUrlRequest* request, const char* data,
		off_t offset, ssize_t size)
	{
		fResult.Write(data, size);
	}

	BMallocIO& IO() { return fResult; }

private:
	BMallocIO fResult;
};


typedef std::map<ImageSize, BUrl> UrlMap;


class ResultListener {
public:
	virtual	void				AddResult(const BString& id,
									const char* info, UrlMap urls) = 0;
};


class ImageLoaderResultListener : public ResultListener {
public:
								ImageLoaderResultListener(
									const BMessenger& target, ImageSize type);
	virtual						~ImageLoaderResultListener();

	virtual	void				AddResult(const BString& id,
									const char* info, UrlMap urls);

private:
			BBitmap*			_LoadImage(const BUrl& url);

private:
			BMessenger			fTarget;
			ImageSize			fType;
};


class Retriever {
public:
								Retriever(ResultListener& listener);
	virtual						~Retriever();

	virtual void				Retrieve(const BString& artist,
									const BString& title) = 0;

protected:
			ResultListener&		fListener;
};


class AmazonRetriever : public Retriever {
public:
								AmazonRetriever(int countryCode,
									const char* country,
									ResultListener& listener);
	virtual						~AmazonRetriever();

	virtual void				Retrieve(const BString& artist,
									const BString& title);

private:
			BUrl				_GetUrl(const BString& id, const char* type);

private:
			int					fCountryCode;
			BString				fCountry;
};


// #pragma mark - Retriever


Retriever::Retriever(ResultListener& listener)
	:
	fListener(listener)
{
}


Retriever::~Retriever()
{
}


// #pragma mark - AmazonRetriever


AmazonRetriever::AmazonRetriever(int countryCode, const char* country,
	ResultListener& listener)
	:
	Retriever(listener),
	fCountryCode(countryCode),
	fCountry(country)
{
}


AmazonRetriever::~AmazonRetriever()
{
}


void
AmazonRetriever::Retrieve(const BString& artist, const BString& title)
{
	BString pattern("<a[^>]*title\\s*=\\s*\"([^\"]+)\"[^>]*"
		"href\\s*=\\s*\"([^\"]+?/dp/([^/]+)/)[^>]+><h2.*?>([^<]+)(</a>)?</span></div></div>");
	//");
	regex_t patternCompiled;
	regmatch_t groups[MAX_GROUPS];
	int patternResult = regcomp(&patternCompiled, pattern.String(),
		REG_EXTENDED | REG_ICASE);
	if (patternResult != 0) {
		printf("bad pattern: %d!\n", patternResult);
		char text[256];
		regerror(patternResult, &patternCompiled, text, sizeof(text));
		printf(" --> %s\n", text);
		return;
	}

	BString urlString("http://www.amazon.%COUNTRY%/gp/search/"
		"?search-alias=popular&sort=relevancerank");
	if (!artist.IsEmpty())
		urlString += "&field-artist=%ARTIST%";
	if (!title.IsEmpty())
		urlString += "&field-title=%TITLE%";
	urlString.ReplaceAll("%COUNTRY%", fCountry);
	urlString.ReplaceAll("%ARTIST%", BUrl::UrlEncode(artist));
	urlString.ReplaceAll("%TITLE%", BUrl::UrlEncode(title));
	printf("URL: %s\n", urlString.String());

	BUrl url(urlString);
	BHttpRequest request(url);

	DataListener listener;
	request.SetListener(&listener);
	request.Run();
	while (request.IsRunning()) {
		snooze(50000);
		putchar('.');
		fflush(stdout);
	}

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());
	printf("Status: %" B_PRId32 " - %s\n", result.StatusCode(),
		result.StatusText().String());

	BString data((const char*)listener.IO().Buffer(),
		listener.IO().BufferLength());
	printf("length: %d\n", (int)listener.IO().BufferLength());
	const char* cursor = data.String();
	int offset = 0;

	for (int index = 0; index < MAX_MATCHES; index++) {
		if (regexec(&patternCompiled, cursor, MAX_GROUPS, groups, 0))
			break;

		BString id;

		for (int groupIndex = 0; groupIndex < MAX_GROUPS; groupIndex++) {
			if (groups[groupIndex].rm_so == -1)
				break;
			if (groupIndex == 0)
				offset = groups[0].rm_eo;

			int length = groups[groupIndex].rm_eo - groups[groupIndex].rm_so;
			BString match(cursor + groups[groupIndex].rm_so, length);
			if (groupIndex)
				printf("%d: match? %s\n", groupIndex, match.String());
			if (groupIndex == 3)
				id = match;
		}

		if (!id.IsEmpty()) {
			UrlMap urls;
			urls.insert(std::make_pair(kOriginal, _GetUrl(id, TYPE_ORIGINAL)));
			urls.insert(std::make_pair(kLarge, _GetUrl(id, TYPE_LARGE)));
			urls.insert(std::make_pair(kThumb, _GetUrl(id, TYPE_THUMB)));
			fListener.AddResult(id, NULL, urls);
		}

printf("offset = %d\n", offset);
		cursor += offset;
	}
	regfree(&patternCompiled);
}


BUrl
AmazonRetriever::_GetUrl(const BString& id, const char* type)
{
	BString urlString;
	urlString.SetToFormat("http://ecx.images-amazon.com/images/P/%s.%02d._%s_",
		id.String(), fCountryCode, type);

	return BUrl(urlString.String());
}


// #pragma mark - ImageLoaderResultListener


ImageLoaderResultListener::ImageLoaderResultListener(const BMessenger& target,
	ImageSize type)
	:
	fTarget(target),
	fType(type)
{
}


ImageLoaderResultListener::~ImageLoaderResultListener()
{
}


void
ImageLoaderResultListener::AddResult(const BString& id, const char* info,
	UrlMap urls)
{
	printf("LOAD: %s\n", urls[fType].UrlString().String());
}


BBitmap*
ImageLoaderResultListener::_LoadImage(const BUrl& url)
{
	BHttpRequest request(url);

	DataListener listener;
	request.SetListener(&listener);
	request.Run();
	while (request.IsRunning()) {
		snooze(50000);
		putchar('.');
		fflush(stdout);
	}

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());
	printf("Status: %" B_PRId32 " d - %s\n", result.StatusCode(),
		result.StatusText().String());

	listener.IO().Seek(0, SEEK_SET);
	return BTranslationUtils::GetBitmap(&listener.IO());
}


// #pragma mark - Query


Query::Query(const BMessenger& target, const BString& artist,
	const BString& title)
	:
	fTarget(target),
	fRetrievers(5, true),
	fListener(NULL),
	fArtist(artist),
	fTitle(title),
	fAbort(false)
{
	fListener = new ImageLoaderResultListener(fTarget, kThumb);

	AddRetriever(new AmazonRetriever(COUNTRY_GERMANY, "de", *fListener));
}


Query::~Query()
{
}


void
Query::AddRetriever(Retriever* retriever)
{
	fRetrievers.AddItem(retriever);
}


void
Query::Run()
{
}


void
Query::Abort()
{
	fAbort = true;
}


/*static*/ status_t
Query::_Run(void* _self)
{
	Query* self = (Query*)_self;
	status_t status = self->_Run();
	delete self;

	return status;
}


status_t
Query::_Run()
{
	for (int32 index = 0; index < fRetrievers.CountItems(); index++)
		fRetrievers.ItemAt(index)->Retrieve(fArtist, fTitle);

	return B_OK;
}

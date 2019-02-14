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

#include "Utility.h"


#define MAX_GROUPS 10
#define MAX_MATCHES 20

#define COUNTRY_GERMANY 3

#define TYPE_ORIGINAL "SCRM"
#define TYPE_LARGE "SCL"
#define TYPE_THUMB "THUMB"


class Retriever {
public:
								Retriever(ResultListener& listener);
	virtual						~Retriever();

	virtual	void				Abort() = 0;
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

	virtual	void				Abort();
	virtual void				Retrieve(const BString& artist,
									const BString& title);

private:
			BUrl				_GetUrl(const BString& id, const char* type);

private:
			int					fCountryCode;
			BString				fCountry;
			bool				fAbort;
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
	fCountry(country),
	fAbort(false)
{
}


AmazonRetriever::~AmazonRetriever()
{
}


void
AmazonRetriever::Abort()
{
	fAbort = true;
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
	while (request.IsRunning() && !fAbort) {
		snooze(50000);
	}
	if (fAbort)
		return;

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
		BString info;

		for (int groupIndex = 0; groupIndex < MAX_GROUPS; groupIndex++) {
			if (groups[groupIndex].rm_so == -1)
				break;

printf("%d. (%d - %d)\n", groupIndex, groups[groupIndex].rm_so, groups[groupIndex].rm_eo);
			if (groupIndex == 3)
				offset = groups[3].rm_eo;

			int length = groups[groupIndex].rm_eo - groups[groupIndex].rm_so;
			BString match(cursor + groups[groupIndex].rm_so, length);
			if (groupIndex)
				printf("%d: match? %s\n", groupIndex, match.String());
			switch (groupIndex) {
				case 1:
					info = match;
					break;
				case 2:
					url = match;
					break;
				case 3:
					id = match;
					break;
			}
		}

		if (!id.IsEmpty()) {
			UrlMap urls;
			urls.insert(std::make_pair(kSource, url));

			urls.insert(std::make_pair(kOriginalImage,
				_GetUrl(id, TYPE_ORIGINAL)));
			urls.insert(std::make_pair(kLargeImage, _GetUrl(id, TYPE_LARGE)));
			urls.insert(std::make_pair(kThumbImage, _GetUrl(id, TYPE_THUMB)));
			fListener.AddResult(id, info, urls);
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


// #pragma mark - Query


Query::Query(const BString& artist, const BString& title)
	:
	fRetrievers(5, true),
	fListeners(5, true),
	fArtist(artist),
	fTitle(title),
	fAbort(false)
{
	printf("THIS NEW %p\n", this);
	AddRetriever(new AmazonRetriever(COUNTRY_GERMANY, "de", *this));
}


Query::~Query()
{
	puts("DELETE QUERY!");
}


void
Query::AddRetriever(Retriever* retriever)
{
	fRetrievers.AddItem(retriever);
	printf("new retriever: %ld\n", fRetrievers.CountItems());
}


void
Query::AddListener(ResultListener* listener)
{
	fListeners.AddItem(listener);
}


void
Query::Run()
{
	printf("retrievers: %ld\n", fRetrievers.CountItems());
	fThread = spawn_thread(&_Run, "query artist/title", B_NORMAL_PRIORITY,
		this);
	if (fThread >= 0)
		resume_thread(fThread);
}


void
Query::Abort(bool wait)
{
	printf("THIS Abort %p\n", this);
	printf("THIS Abort %ld\n", fRetrievers.CountItems());
	fAbort = true;
	for (int32 index = 0; index < fRetrievers.CountItems(); index++)
		fRetrievers.ItemAt(index)->Abort();

	if (wait)
		wait_for_thread(fThread, NULL);
}


void
Query::AddResult(const BString& id, const char* info, UrlMap urls)
{
	if (fAbort)
		return;

	for (int32 i = 0; i < fListeners.CountItems(); i++) {
		fListeners.ItemAt(i)->AddResult(id, info, urls);
	}
}


/*static*/ status_t
Query::_Run(void* _self)
{
	Query* self = (Query*)_self;
	return self->_Run();
}


status_t
Query::_Run()
{
	printf("THIS Run %p\n", this);
	printf("run retriever: %ld\n", fRetrievers.CountItems());

	for (int32 index = 0; index < fRetrievers.CountItems(); index++)
		fRetrievers.ItemAt(index)->Retrieve(fArtist, fTitle);

	return B_OK;
}

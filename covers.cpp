/* covers - Downloads cover images from Amazon.
 *
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <HttpRequest.h>
#include <TranslationUtils.h>

#include <regex.h>
#include <stdio.h>


#define MAX_GROUPS 10
#define MAX_MATCHES 10

#define COUNTRY_GERMANY 3

#define TYPE_ORIGINAL "SCRM"
#define TYPE_LARGE "SCL"
#define TYPE_THUMB "THUMB"


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


BBitmap*
loadImage(int countryCode, const BString& id, const char* type)
{
	BString urlString;
	urlString.SetToFormat("http://ecx.images-amazon.com/images/P/%s.%02d._%s_",
		id.String(), countryCode, type);

	BUrl url(urlString.String());
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
	printf("Status: %d - %s\n", result.StatusCode(),
		result.StatusText().String());

	listener.IO().Seek(0, SEEK_SET);
	return BTranslationUtils::GetBitmap(&listener.IO());
}


void
getImageFromAmazon(const char* country, int countryCode, const BString& artist,
	const BString& title)
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
	urlString.ReplaceAll("%COUNTRY%", country);
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
	printf("Status: %d - %s\n", result.StatusCode(),
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
			if (groups[groupIndex].rm_so == (size_t)-1)
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
			BBitmap* bitmap = loadImage(COUNTRY_GERMANY, id, TYPE_ORIGINAL);
			if (bitmap != NULL)
				puts("got image!");
			else
				puts("nay!");
		}

printf("offset = %d\n", offset);
		cursor += offset;
	}
	regfree(&patternCompiled);
}


int
main()
{
	BApplication application("application/x-vnd.pinc-covers");
	getImageFromAmazon("de", COUNTRY_GERMANY, "toundra", "");
}

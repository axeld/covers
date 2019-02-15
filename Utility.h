/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <HttpRequest.h>


class BBitmap;


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


BBitmap* FetchImage(const BUrl& url);


#endif // UTILITY_H

/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef QUERY_H
#define QUERY_H


#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>
#include <Url.h>

#include <map>


class BUrl;
class Retriever;


enum UrlType {
	kSource,
	kThumbImage,
	kLargeImage,
	kOriginalImage,
};


typedef std::map<UrlType, BUrl> UrlMap;


class ResultListener {
public:
	virtual	void				AddResult(const BString& id,
									const char* info, UrlMap urls) = 0;
};


class Query : public ResultListener {
public:
								Query(const BString& artist,
									const BString& title);
	virtual						~Query();

			void				AddRetriever(Retriever* retriever);
			void				AddListener(ResultListener* listener);

			void				Run();
			void				Abort(bool wait = true);

	virtual	void				AddResult(const BString& id,
									const char* info, UrlMap urls);

private:
	static	status_t			_Run(void* self);
			status_t			_Run();

private:
			thread_id			fThread;
			BObjectList<Retriever>	fRetrievers;
			BObjectList<ResultListener>	fListeners;
			BString				fArtist;
			BString				fTitle;
			bool				fAbort;
};


#endif // QUERY_H

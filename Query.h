/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef QUERY_H
#define QUERY_H


#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>


class ResultListener;
class Retriever;


class Query {
public:
								Query(const BMessenger& target,
									const BString& artist,
									const BString& title);
								~Query();

			void				AddRetriever(Retriever* retriever);
			void				Run();
			void				Abort();

private:
	static	status_t			_Run(void* self);
			status_t			_Run();

private:
			BMessenger			fTarget;
			BObjectList<Retriever>	fRetrievers;
			ResultListener*		fListener;
			BString				fArtist;
			BString				fTitle;
			bool				fAbort;
};


#endif // QUERY_H

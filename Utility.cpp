/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "Utility.h"

#include <Bitmap.h>
#include <DataIO.h>
#include <TranslationUtils.h>

#include <stdio.h>


BBitmap*
FetchImage(const BUrl& url)
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

/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef COVER_WINDOW_H
#define COVER_WINDOW_H


#include <Window.h>


class BGridView;
class BTextControl;

class Query;


class CoverWindow : public BWindow {
public:
								CoverWindow();
	virtual						~CoverWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_AbortQuery();

private:
			BGridView*			fMainView;
			BTextControl*		fArtistControl;
			BTextControl*		fTitleControl;
			Query*				fQuery;
			int32				fCount;
};


#endif // COVER_WINDOW_H

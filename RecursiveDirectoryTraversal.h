#include <windows.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

//Deal with outdated headers
typedef struct _NEW_WIN32_FIND_DATAA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR cFileName[MAX_PATH];
	CHAR cAlternateFileName[14];
	//Careful with next fields, their existance is conditional in new headers
	DWORD dwFileType;
	DWORD dwCreatorType;
	WORD wFinderFlags;

	inline WIN32_FIND_DATAA* cast() {
		return (WIN32_FIND_DATAA*)this;
	}
} NEW_WIN32_FIND_DATAA, *PNEW_WIN32_FIND_DATAA, *LPNEW_WIN32_FIND_DATAA;

class RecursiveDirectoryTraversal {
	protected:
		struct TraversalStackElement {
			HANDLE searchHandle;
			short unsigned int pathLength;
			TraversalStackElement* previous;
		};

		TraversalStackElement firstStackElement;
		TraversalStackElement* stackHead = nullptr;
		NEW_WIN32_FIND_DATAA searchResult;
		char path[MAX_PATH];

		//=========================================================================================

		inline bool getNextObject() {
			return FindNextFileA(stackHead->searchHandle, searchResult.cast());
		}

		//"." - this folder - and ".." - owning folder - are returned among results
		inline bool currentObjectIsServiceObject() {
			return (searchResult.cFileName[0] == '.')
			       && (
			           (searchResult.cFileName[1] == '\0')
			           ||
			           (searchResult.cFileName[1] == '.' && searchResult.cFileName[2] == '\0')
			       );
		}

		bool skipServiceObjects() {
			while(currentObjectIsServiceObject()) {
				if(!getNextObject()) {
					return false;
				}
			}
			return true;
		}

		void buildLastFoundObjectPath() {
			short unsigned int i;
			for(i = 0; searchResult.cFileName[i] != '\0'; i++) {
				path[stackHead->pathLength + i] = searchResult.cFileName[i];
			}
			path[stackHead->pathLength + i] = '\0';
		}

		bool push() {
			short unsigned int i;
			for (i = stackHead->pathLength; path[i] != '\0'; i++) {}
			path[i] = '\\';
			path[i + 1] = '*';
			path[i + 2] = '\0';

			TraversalStackElement* newElement = new TraversalStackElement();
			newElement->pathLength = i + 1;
			newElement->searchHandle = FindFirstFileA(path, searchResult.cast());
			newElement->previous = stackHead;
			stackHead = newElement;

			path[i + 1] = '\0';

			if(skipServiceObjects()) {
				buildLastFoundObjectPath();
				return true;
			} else {
				pop();
				return false;
			}
		}

		void pop() {
			TraversalStackElement* oldHead = stackHead;
			FindClose(oldHead->searchHandle);
			stackHead = stackHead->previous;

			if(stackHead) {
				path[stackHead->pathLength] = '\0';
			}

			if(oldHead != &firstStackElement) {
				delete oldHead;
			}
		}

		//=========================================================================================

	public:

		inline bool currentObjectIsDirectory() {
			return GetFileAttributesA(path) & FILE_ATTRIBUTE_DIRECTORY;
		}

		inline const char* currentObjectPath() {
			return path;
		}

		//=========================================================================================

		bool begin(const char* path) {
			short unsigned int i;
			for (i = 0; path[i] != '\0'; i++) {
				this->path[i] = path[i];
			}
			this->path[i] = '\\';
			this->path[i + 1] = '*';
			this->path[i + 2] = '\0';

			stackHead = &firstStackElement;
			firstStackElement.previous = nullptr;
			firstStackElement.pathLength = i + 1;
			firstStackElement.searchHandle = FindFirstFileA(this->path, searchResult.cast());
			if(firstStackElement.searchHandle == INVALID_HANDLE_VALUE) {
				FindClose(firstStackElement.searchHandle);
				return false;
			}
			
			this->path[i + 1] = '\0';
			if(!skipServiceObjects()) {
				FindClose(firstStackElement.searchHandle);
				return false;
			}
			
			buildLastFoundObjectPath();
			return true;
		}

		bool stepOver() {
			while (stackHead != nullptr) {
				while (getNextObject()) {
					if (!currentObjectIsServiceObject()) {
						buildLastFoundObjectPath();
						return true;
					}
				}
				pop();
			}
			return false;
		}

		bool stepInto() {
			if(currentObjectIsDirectory()) {
				if(push()) {
					return true;
				}
			}

			return stepOver();
		}

		void end() {
			while(stackHead != &firstStackElement) {
				TraversalStackElement* oldHead = stackHead;
				FindClose(stackHead->searchHandle);
				stackHead = stackHead->previous;
				delete oldHead;
			}
			FindClose(firstStackElement.searchHandle);
		}

		~RecursiveDirectoryTraversal() {
			if (stackHead != nullptr) end();
		}
};

void RecursiveDirectoryTraversalSimpleTest() {
	const char* path = "C:\\Windows\\Boot";

	RecursiveDirectoryTraversal traversal;

	if(traversal.begin(path)) {
		do {
			std::cout << traversal.currentObjectPath() << '\n';
		} while(traversal.stepInto());
	}
}
#include "gmock/gmock.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Controller/MainController.h"
#include "Persistence/JsonFileStore.h"
#include "Repository/JsonOrderRepository.h"
#include "Repository/JsonProductionQueueRepository.h"
#include "Repository/JsonSampleRepository.h"
#include "View/ConsoleView.h"

using namespace testing;

int main() {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();

#else
	// 소스/실행 문자 집합을 UTF-8로 컴파일(/utf-8)했으므로,
	// 콘솔 입출력 코드페이지도 UTF-8로 맞춰야 한글이 정상 출력된다(ConsoleMVC/DataPersistence PoC 관례).
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	// 구체 클래스를 아는 곳은 이 main.cpp 한 곳뿐이다(design.md §3, §4.2 DIP).
	JsonFileStore store("data/store.json");
	const bool dataLoadFailed = store.isCorrupted();
	JsonSampleRepository sampleRepo(store);
	JsonOrderRepository orderRepo(store);
	JsonProductionQueueRepository productionQueueRepo(store);
	ConsoleView view;

	MainController controller(dataLoadFailed, sampleRepo, orderRepo, productionQueueRepo, view);
	controller.run();

	return 0;
#endif
}
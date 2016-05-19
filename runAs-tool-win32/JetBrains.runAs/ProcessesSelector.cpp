#include "stdafx.h"
#include "ProcessesSelector.h"
#include <queue>
#include "IProcess.h"
#include "SelfTest.h"

ProcessesSelector::ProcessesSelector()
{
}

const Result<queue<const IProcess*>> ProcessesSelector::SelectProcesses(const Settings& settings) const
{
	Trace trace(settings.GetLogLevel());
	trace < L"ProcessesSelector::SelectProcesses";

	const SelfTest selfTest;
	auto selfTestStatisticResult = selfTest.GetStatistic(settings);
	if (selfTestStatisticResult.HasError())
	{
		return selfTestStatisticResult.GetError();
	}

	auto selfTestStatistic = selfTestStatisticResult.GetResultValue();
	auto hasMoreThenHighIntegrityLevel = selfTestStatistic.GetIntegrityLevel() == INTEGRITY_LEVEL_SYSTEM || selfTestStatistic.GetIntegrityLevel() == INTEGRITY_LEVEL_HIGH;
	auto requiredMoreThenHighIntegrityLevel = settings.GetIntegrityLevel() == INTEGRITY_LEVEL_SYSTEM || settings.GetIntegrityLevel() == INTEGRITY_LEVEL_HIGH;

	queue<const IProcess*> processes;	
	// For Local System services (INTEGRITY_LEVEL_SYSTEM) and services under admin (INTEGRITY_LEVEL_HIGH)
	// With SeAssignPrimaryTokenPrivilege
	// Should be elevated admin, if arg "-il" is "auto" or "high" or "system"
	if (hasMoreThenHighIntegrityLevel && selfTestStatistic.HasSeAssignPrimaryTokenPrivilege())
	{
		trace < L"ProcessesSelector::SelectProcesses push ProcessAsUser";
		processes.push(requiredMoreThenHighIntegrityLevel ? &_processAsUserElevated : &_processAsUser);
	}

	// For interactive admin accounts (not a service)
	// Could be elevated admin, if arg "-il" is "auto" or "high" or "system"
	if (requiredMoreThenHighIntegrityLevel && selfTestStatistic.HasLogonSid() && selfTestStatistic.HasAdministrativePrivileges())
	{
		trace < L"ProcessesSelector::SelectProcesses push ProcessWithLogonElevated";
		processes.push(&_processWithLogonElevated);
	}

	// For interactive Windows accounts
	// Can't be elevated
	trace < L"ProcessesSelector::SelectProcesses push ProcessWithLogon";
	processes.push(&_processWithLogon);
	return processes;
}

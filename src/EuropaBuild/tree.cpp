#include "EuropaBuild/tree.hpp"

#include <unordered_map>
#include <queue>

namespace EuropaBuild
{

	BuildTree::BuildTree(const Targets& targets, GeneralBuildSettings generalSettings)
		: generalSettings(generalSettings)
	{
		// sort target dependencies so they will be built in order

		using TargetPtr = std::shared_ptr<const Target>;
		std::unordered_map<std::string, int> inDegree;
		std::unordered_map<std::string, TargetPtr> nameToTarget;
		// maps a dependency to the targets that depend on it (dependency -> dependents)
		std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
		// this is only used to determine which targets are "final", i.e. ones that no others depend on
		std::vector<std::string> allDependencies;

		for (const TargetPtr& target : targets)
		{
			nameToTarget[target->name] = target;
			inDegree[target->name] = 0;
			allDependencies.insert(allDependencies.end(), target->depends.begin(), target->depends.end());
		}

		// build adjacency list and in-degrees
		for (const TargetPtr& target : targets)
		{
			// U is the current target
			const std::string& U = target->name;

			for (const std::string& V : target->depends)
			{
				// V is a dependency of U, so V must be built before U
				if (nameToTarget.find(V) == nameToTarget.end())
				{
					throw std::runtime_error("Failed to resolve dependency graph, target " + U + " depends on non-existent target " + V);
				}

				// V is a prerequisite, and U is a dependent of V
				// add U to V's adjacency list (what V needs to notify when built)
				adjacencyList[V].push_back(U);

				// increment U's in-degree because V is a dependency for U
				inDegree[U]++;
			}
		}

		// Kahn's algorithm
		std::queue<std::string> q;

		for (const auto& pair : inDegree)
		{
			if (pair.second == 0)
			{
				q.push(pair.first);
			}
		}

		while (!q.empty())
		{
			std::string current_name = q.front();
			q.pop();

			targetsOrdered.push_back(nameToTarget.at(current_name));

			// iterate all targets that depend on the current one
			if (adjacencyList.count(current_name))
			{
				for (const std::string& dependent_name : adjacencyList.at(current_name))
				{
					inDegree[dependent_name]--;

					// if in-degree hits 0 it means all dependencies are satisfied
					if (inDegree[dependent_name] == 0) {
						q.push(dependent_name);
					}
				}
			}
		}

		if (targetsOrdered.size() != targets.size())
		{
			throw std::runtime_error("Failed to resolve dependency graph, circular dependency detected");
		}

		for (const TargetPtr& target : targetsOrdered)
		{
			auto it = std::find(allDependencies.begin(), allDependencies.end(), target->name);
			if (it == allDependencies.end())
				targetsThatAreFinalProducts.push_back(target);
		}
	}


	Targets::const_iterator BuildTree::begin() const { return targetsOrdered.begin(); }
	Targets::const_iterator BuildTree::end() const { return targetsOrdered.end(); }
	size_t BuildTree::size() const { return targetsOrdered.size(); }

	std::string BuildTree::getWholeDependecyTreeAsString() const
	{
		std::string s;
		size_t iterationDepth = 0;
		for (const auto& target : targetsOrdered)
		{
			s += "Target: " + getDependecyTreeForTarget(*target, iterationDepth) + "\n";
		}
		if (!s.empty() && s.back() == '\n')
		{
			s.pop_back(); // remove trailing newlines
			s.pop_back();
		}
		return s;
	}

	std::string BuildTree::getDependecyTreeForTarget(const Target& t, size_t& iterationDepth) const
	{
		iterationDepth++;
		std::string s;
		for (auto i = 0; i < iterationDepth - 1; i++)
			s += (i != iterationDepth - 2) ? "  " : " |"; // indent accorrding to dependency nesting level
		s += t.name + "\n";
		for (const std::string& d : t.depends)
		{
			Targets::const_iterator it = std::find_if(targetsOrdered.begin(), targetsOrdered.end(),
				[d](const std::shared_ptr<const Target>& candidate)
				{ return candidate->name == d; });
			s += (it != targetsOrdered.end()) ? getDependecyTreeForTarget(*it->get(), iterationDepth) : "???";
		}
		iterationDepth--;
		return s;
	}

} // namespace EuropaBuild


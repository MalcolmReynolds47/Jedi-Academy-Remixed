// Portable camera feed selection within one scene. GPL-2.0-only.
#ifndef JAR_PORTAL_MONITOR_FEEDS_H
#define JAR_PORTAL_MONITOR_FEEDS_H

#include <algorithm>

template<class Surface>
class PortableMonitorCandidates {
public:
	void Add(Surface& surface, const jar::portal::View& panel) {
		const float coverage = R_PortablePanelCoverage(panel);
		if (coverage > 0 && count < candidates.size()) {
			candidates[count] = {&surface, coverage, count};
			++count;
		}
	}

	template<class Visit>
	void VisitLargestFirst(Visit visit) {
		auto end = candidates.begin() + count;
		std::sort(candidates.begin(), end, [](const Candidate& a, const Candidate& b) {
			if (a.coverage != b.coverage)
				return a.coverage > b.coverage;
			return a.order < b.order;
		});
		for (auto current = candidates.begin(); current != end; ++current)
			visit(*current->surface);
	}

private:
	struct Candidate {
		Surface* surface;
		float coverage;
		unsigned order;
	};
	std::array<Candidate, jar::portal::frameLimit> candidates;
	unsigned count = 0;
};

class PortableMonitorFeeds {
public:
	struct Selection {
		qhandle_t shader = 0;
		bool rendered = false;
	};

	template<class Render>
	Selection Select(const jar::portal::View& panel, Render render) {
		for (int slot = 0; slot < count; ++slot) {
			const auto& feed = feeds[slot];
			if (SameView(panel, *feed.panel))
				return {feed.shader, false};
		}

		if (count == JAR_PORTAL_CAMERA_FEEDS || !render(count))
			return {};

		const auto shader = RE_RegisterShader(jar::portal::monitorMaterials[count]);
		feeds[count++] = {&panel, shader};
		return {shader, true};
	}

private:
	struct Feed {
		const jar::portal::View* panel;
		qhandle_t shader;
	};

	static bool SameView(const jar::portal::View& a, const jar::portal::View& b) {
		// Compare aspect independently of panel size and without division rounding.
		if (double(a.width) * b.height != double(b.width) * a.height)
			return false;
		if (!VectorCompare(a.cameraOrigin, b.cameraOrigin))
			return false;
		for (int axis = 0; axis < 3; ++axis)
			if (!VectorCompare(a.cameraAxis[axis], b.cameraAxis[axis]))
				return false;
		return true;
	}

	Feed feeds[JAR_PORTAL_CAMERA_FEEDS]{};
	int count = 0;
};

#endif

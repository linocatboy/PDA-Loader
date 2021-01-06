#ifdef _600
#pragma once
#include "EmulatorComponent.h"
#include <string>

namespace ELAC::Components
{
	class GLComponent : public EmulatorComponent
	{
	public:
		GLComponent();
		~GLComponent();

		virtual const char* GetDisplayName() override;

		virtual void Initialize() override;
		virtual void Update() override;
		virtual void UpdateInput() override;
		
		double* fbAspectRatio;

		std::string inputString;
	};
}
#endif

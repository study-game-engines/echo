#pragma once

#include "engine/core/util/Array.hpp"
#include "../particle_system.h"

#ifdef ECHO_EDITOR_MODE

#include "ui_particle_system_panel.h"

namespace Echo
{
	class ParticleSystemPanel : public QDockWidget, Ui_ParticleSystemPanel
	{
		typedef vector<QGraphicsItem*>::type QGraphicsItemArray;
		typedef vector<QGraphicsProxyWidget*>::type QGraphicsWidgetArray;

	public:
		ParticleSystemPanel(Object* obj);
		virtual ~ParticleSystemPanel();

		// update
		void update();

	public:
		// new
		void onNewAtla();

		// import
		void onImport();
		void onImportFromImages();

		// split
		void onSplit();
		void onSelectItem();

		// change name
		void onChangedAtlaName();

		// refresh list
		void refreshUiDisplay();
		void refreshAtlaList();
		void refreshImageDisplay();

		// clear
		void clearImageItemAndBorder();

	protected:
		ParticleSystem*				m_particleSystem;
		QMenu*						m_importMenu = nullptr;
		QGraphicsScene*				m_graphicsScene = nullptr;
		QGraphicsItem*				m_imageItem = nullptr;
		QGraphicsItem*				m_imageBorder = nullptr;
		QGraphicsItem*				m_atlaBorder = nullptr;
	};
}

#endif

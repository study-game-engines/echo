#include "timeline_panel.h"
#include "engine/core/editor/editor.h"
#include "engine/core/editor/qt/QWidgets.h"
#include "engine/core/base/class_method_bind.h"
#include "engine/core/util/PathUtil.h"
#include "engine/core/util/StringUtil.h"
#include "engine/core/main/Engine.h"
#include "../../anim_timeline.h"
#include "timeline_header.h"
#include "timeline_object_bar.h"
#include "timeline_keyframe_bar.h"

#ifdef ECHO_EDITOR_MODE

#include <QtWidgets/QtWidgets>

namespace Echo
{
	class TimelinePanelUtil
	{
	public:
		// is a object item
		static bool isObject(QTreeWidgetItem* item)
		{
			String text = item->text(0).toStdString().c_str();
			String userData = item->data(0, Qt::UserRole).toString().toStdString().c_str();
			return userData == "object" ? true : false;
		}

		// is a property item
		static bool isProperty(QTreeWidgetItem* item)
		{
			String text = item->text(0).toStdString().c_str();
			String userData = item->data(0, Qt::UserRole).toString().toStdString().c_str();
			return userData == "property" ? true : false;
		}

		// get property chain and object path
		static void queryNodePathAndPropertyChain(QTreeWidgetItem* item, String& nodePath, StringArray& propertyChain)
		{
			propertyChain.clear();

			String userData = item->data(0, Qt::UserRole).toString().toStdString().c_str();
			if (userData == "property")
			{
				QTreeWidgetItem* currItem = item;
				while (isProperty(currItem))
				{
					propertyChain.insert(propertyChain.begin(), currItem->text(0).toStdString().c_str());
					currItem = currItem->parent();
				}

				nodePath = currItem->text(0).toStdString().c_str();
			}
			else
			{
				propertyChain.clear();
				nodePath = item->text(0).toStdString().c_str();
			}
		}

		// get value
		static bool getProertyValue(Timeline* timeline, String& objectPath, const StringArray& propertyChain, Variant& oVar)
		{
			Object* lastObject = timeline->getLastObject(objectPath, propertyChain);
			if (lastObject)
			{
				Class::getPropertyValue(lastObject, propertyChain.back(), oVar);
				return true;
			}

			return false;
		}
	};

	TimelinePanel::TimelinePanel(Object* obj)
		: m_addObjectMenu(nullptr)
		, m_nodeTreeWidgetWidth(0)
	{
		setupUi(this);

		m_timeline = ECHO_DOWN_CAST<Timeline*>(obj);

		m_treeWidgetHeader = new QTimelineHeader(Qt::Orientation::Horizontal, this);
		m_treeWidget->setHeader(m_treeWidgetHeader);
		m_treeWidget->setHeaderLabels({"", ""});
		m_treeWidget->setColumnWidth(0, 265);
		m_treeWidget->header()->setStretchLastSection(true);

		// set icons
		setToolbuttonIcon( NewClip, "engine/modules/anim/editor/icon/new.png");
		setToolbuttonIcon( DuplicateClip, "engine/modules/anim/editor/icon/duplicate.png");
		setToolbuttonIcon( DeleteClip, "engine/modules/anim/editor/icon/delete.png");

		// connect signal slots
		EditorApi.qConnectWidget(NewClip, QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onNewClip));
		EditorApi.qConnectWidget(DuplicateClip, QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onDuplicateClip));
		EditorApi.qConnectWidget(DeleteClip, QSIGNAL(clicked()), this, createMethodBind(&TimelinePanel::onDeleteClip));
		EditorApi.qConnectWidget(m_clipLengthLineEdit, QSIGNAL(editingFinished()), this, createMethodBind(&TimelinePanel::onCurrentEditAnimLengthChanged));
		EditorApi.qConnectWidget(m_clipsComboBox, QSIGNAL(editTextChanged(const QString &)), this, createMethodBind(&TimelinePanel::onRenameClip));
		EditorApi.qConnectWidget(m_clipsComboBox, QSIGNAL(currentIndexChanged(int)), this, createMethodBind(&TimelinePanel::onCurrentEditAnimChanged));
		EditorApi.qConnectWidget(m_treeWidget, QSIGNAL(itemClicked(QTreeWidgetItem*, int)), this, createMethodBind(&TimelinePanel::onSelectItem));
	}

	void TimelinePanel::setToolbuttonIcon(QToolButton* button, const String& iconPath)
	{
		if (button)
		{
			String fullPath = Engine::instance()->getRootPath() + iconPath;
			button->setIcon(QIcon(fullPath.c_str()));
		}
	}

	void TimelinePanel::syncDataToEditor()
	{
		// update display
		syncClipListDataToEditor();
	}

	void TimelinePanel::onNewClip()
	{
		if (m_timeline)
		{
			AnimClip* animClip = EchoNew(AnimClip);
			animClip->m_name = getNewClipName();
			m_timeline->addClip(animClip);

			syncClipListDataToEditor();

			// set current edit anim clip
			setCurrentEditAnim(animClip->m_name.c_str());
		}
	}

	void TimelinePanel::update()
	{
	}

	void TimelinePanel::onDuplicateClip()
	{
		String currentAnim = m_clipsComboBox->currentText().toStdString().c_str();
		if (!currentAnim.empty() && m_timeline)
		{
			AnimClip* animClip = m_timeline->getClip(currentAnim.c_str());
			AnimClip* animClipDuplicate = animClip->duplicate();
			m_timeline->generateUniqueAnimName(currentAnim + " Duplicate ", animClipDuplicate->m_name);
			m_timeline->addClip(animClipDuplicate);

			syncClipListDataToEditor();

			// set current edit anim clip
			setCurrentEditAnim(animClipDuplicate->m_name.c_str());
		}
	}

	void TimelinePanel::onDeleteClip()
	{
		String currentAnim = m_clipsComboBox->currentText().toStdString().c_str();
		if (!currentAnim.empty() && m_timeline)
		{
			m_timeline->deleteClip(currentAnim.c_str());

			syncClipListDataToEditor();

			if (m_timeline->getClipCount() > 0)
			{
				setCurrentEditAnim(m_timeline->getClip(0)->m_name.c_str());
			}
		}
	}

	void TimelinePanel::onRenameClip()
	{
		String currentAnimName = m_clipsComboBox->currentText().toStdString().c_str();
		if (!currentAnimName.empty() && m_timeline)
		{
			int currentIndex = m_clipsComboBox->currentIndex();
			m_timeline->renameClip(currentIndex, currentAnimName.c_str());

			m_clipsComboBox->setItemText(currentIndex, currentAnimName.c_str());
			setCurrentEditAnim(currentAnimName.c_str());
		}
	}

	void TimelinePanel::onAddObject()
	{
		if (!m_addObjectMenu)
		{
			m_addObjectMenu = new QMenu(this);

			m_addObjectMenu->addAction(m_actionAddNode);
			m_addObjectMenu->addAction(m_actionAddSetting);
			m_addObjectMenu->addAction(m_actionAddResource);

			EditorApi.qConnectAction(m_actionAddNode, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddNode));
			EditorApi.qConnectAction(m_actionAddSetting, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddSetting));
			EditorApi.qConnectAction(m_actionAddResource, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddResource));
		}

		m_addObjectMenu->exec(QCursor::pos());
	}

	void TimelinePanel::onAddNode()
	{
		Echo::String path = Editor::instance()->selectANodeObject();
		if (!path.empty())
		{
			Node* node = m_timeline->getNode(path.c_str());
			if (node)
			{
				String relativePath = node->getNodePathRelativeTo(m_timeline);
				m_timeline->addObject(m_currentEditAnim, Timeline::Node, relativePath);
			}

			syncClipNodeDataToEditor();
		}
	}

	void TimelinePanel::onAddSetting()
	{
		Echo::String path = Editor::instance()->selectASettingObject();
		if (!path.empty())
		{
			m_timeline->addObject(m_currentEditAnim, Timeline::Setting, path);

			syncClipNodeDataToEditor();
		}
	}

	void TimelinePanel::onAddResource()
	{
		Echo::String path = Editor::instance()->selectAResObject("");
		if (!path.empty())
		{
			m_timeline->addObject(m_currentEditAnim, Timeline::Resource, path);

			syncClipNodeDataToEditor();
		}
	}

	String TimelinePanel::getNewClipName()
	{
		if (m_timeline)
		{
			Echo::String newName;
			m_timeline->generateUniqueAnimName("Anim ", newName);
			return newName;
		}

		return StringUtil::BLANK;
	}

	void TimelinePanel::syncClipTimeLength()
	{
		AnimClip* clip = m_timeline->getClip(m_currentEditAnim.c_str());
		m_clipLengthLineEdit->setText(clip ? StringUtil::Format("%d", clip->m_length).c_str() : "");
	}

	void TimelinePanel::syncClipListDataToEditor()
	{
		if (m_timeline)
		{
			if (m_clipsComboBox)
			{
				m_clipsComboBox->clear();

				const StringOption& anims = m_timeline->getAnim();
				for (const String& animName : anims.getOptions())
				{
					m_clipsComboBox->addItem(animName.c_str());
				}
			}
		}
	}

	void TimelinePanel::syncClipNodeDataToEditor()
	{
		if (m_treeWidget)
		{
			m_treeWidget->clear();

			AnimClip* clip = m_timeline->getClip(m_currentEditAnim.c_str());
			if (clip)
			{
				QTreeWidgetItem* rootItem = m_treeWidget->invisibleRootItem();
				if (rootItem)
				{
					for (AnimObject* animNode : clip->m_objects)
					{
						const Timeline::ObjectUserData& userData = any_cast<Timeline::ObjectUserData>(animNode->m_userData);
						Node* node = m_timeline->getNode(userData.m_path.c_str());
						if (node)
						{
							QTreeWidgetItem* objectItem = new QTreeWidgetItem;
							objectItem->setData(0, Qt::UserRole, "object");
							rootItem->addChild(objectItem);

							QTimelineObjectBar* objectBar = new QTimelineObjectBar(objectItem);
							objectBar->setText(userData.m_path.c_str());
							m_treeWidget->setItemWidget(objectItem, 0, objectBar);

							m_treeWidget->setItemWidget(objectItem, 1, new QTimelineKeyFrameBar(objectItem));

							for (AnimProperty* property : animNode->m_properties)
							{
								syncPropertyDataToEditor(objectItem, property, StringUtil::Split(property->m_name));
							}
						}
					}
				}
			}
		}
	}

	void TimelinePanel::syncPropertyDataToEditor(QTreeWidgetItem* parentItem, AnimProperty* property, const StringArray& propertyChain)
	{
		if (propertyChain.size() > 0)
		{
			String propertyName = propertyChain.front();
			QTreeWidgetItem* propertyItem = nullptr;

			// is exist
			for (i32 i = 0; i < parentItem->childCount(); i++)
			{
				QTreeWidgetItem* child = parentItem->child( i);
				if (child && child->text(0).toStdString()==propertyName)
				{
					propertyItem = child;
				}
			}

			// create new
			if (!propertyItem)
			{
				propertyItem = new QTreeWidgetItem;
				propertyItem->setText( 0, propertyName.c_str());
				propertyItem->setData( 0, Qt::UserRole, "property");
				propertyItem->setExpanded(true);
				parentItem->addChild(propertyItem);

				if (property->getType() == AnimProperty::Type::Object)
				{
					propertyItem->setData( 1, Qt::UserRole, "property");
					propertyItem->setIcon(1, QIcon((Engine::instance()->getRootPath() + "engine/modules/anim/editor/icon/add.png").c_str()));

					propertyItem->setData( 2, Qt::UserRole, "property");
					propertyItem->setBackgroundColor(2, Qt::red);
				}
				else
				{
					propertyItem->setData(2, Qt::UserRole, "property");
					propertyItem->setBackgroundColor(2, Qt::red);
				}
			}

			// add last property
			if (propertyChain.size() > 1)
			{
				StringArray lastPropertyChain = propertyChain;
				lastPropertyChain.erase(lastPropertyChain.begin());
				syncPropertyDataToEditor(propertyItem, property, lastPropertyChain);
			}
		}
	}

	void TimelinePanel::addNodePropertyToEditor()
	{

	}

	void TimelinePanel::setCurrentEditAnim(const char* animName)
	{
		if (m_clipsComboBox)
		{
			int index = m_timeline->getClipIndex(animName);
			m_clipsComboBox->setCurrentIndex(index);

			m_currentEditAnim = animName;

			// sync clip node data to editor
			syncClipNodeDataToEditor();

			// sync clip time length data to editor
			syncClipTimeLength();
		}
	}

	void TimelinePanel::onSelectItem()
	{
		//QTreeWidgetItem* item = qTreeWidgetCurrentItem(qFindChild(m_ui, "m_nodeTreeWidget"));
		int column = m_treeWidget->currentColumn();
		if (column == 1)
		{
			onAddProperty();
		}
		else if (column == 0)
		{
			onSelectProperty();
		}
	}

	void TimelinePanel::onAddProperty()
	{
		QTreeWidgetItem* item = m_treeWidget->currentItem();
		int column = m_treeWidget->currentColumn();
		if (column == 1)
		{
			String userData = item->data(column, Qt::UserRole).toString().toStdString().c_str();
			Echo::StringArray propertyChain;
			if (userData == "object")
			{
				String text = item->text(0).toStdString().c_str();
				Node* node = m_timeline->getNode(text.c_str());
				if (node)
				{
					String nodeRelativePath = node->getNodePathRelativeTo(m_timeline);
					String propertyName = Editor::instance()->selectAProperty(node);
					if (!propertyName.empty())
					{
						propertyChain.insert(propertyChain.begin(), propertyName);
						AnimProperty::Type propertyType = m_timeline->getAnimPropertyType(nodeRelativePath, propertyChain);
						if (propertyType != AnimProperty::Type::Unknown)
						{
							m_timeline->addProperty(m_currentEditAnim, nodeRelativePath, propertyChain, propertyType);

							// addNodePropertyToEditor;
							QTreeWidgetItem* propertyItem = new QTreeWidgetItem;
							propertyItem->setText( 0, propertyName.c_str());
							propertyItem->setData( 0, Qt::UserRole, "property");
							item->setExpanded(true);
							item->addChild(propertyItem);

							if (propertyType == AnimProperty::Type::Object)
							{
								propertyItem->setData(1, Qt::UserRole, "property");
								propertyItem->setIcon(1, QIcon((Engine::instance()->getRootPath() + "engine/modules/anim/editor/icon/add.png").c_str()));
							}
						}
					}
				}
			}
			else if (userData == "property")
			{
				QTreeWidgetItem* nodeItem = item->parent();
				propertyChain.insert(propertyChain.begin(), item->text(0).toStdString().c_str());
				while (TimelinePanelUtil::isProperty(nodeItem))
				{
					nodeItem = nodeItem->parent();
					propertyChain.insert(propertyChain.begin(), nodeItem->text(0).toStdString().c_str());
				}

				String text = nodeItem->text(0).toStdString().c_str();
				Node* node = m_timeline->getNode(text.c_str());
				Object* propertyObject = node;
				for (Echo::String propertyName : propertyChain)
				{
					Echo::Variant propertyValue;
					Class::getPropertyValue(propertyObject, propertyName, propertyValue);

					propertyObject = propertyValue.toObj();
				}

				String propertyName = Editor::instance()->selectAProperty(propertyObject);
				if (!propertyName.empty())
				{
					propertyChain.emplace_back(propertyName);

					AnimProperty::Type propertyType = m_timeline->getAnimPropertyType(node->getNodePathRelativeTo(m_timeline), propertyChain);
					if (propertyType != AnimProperty::Type::Unknown)
					{
						if(m_timeline->addProperty(m_currentEditAnim, node->getNodePathRelativeTo(m_timeline), propertyChain, propertyType))
						{ 
							// addNodePropertyToEditor;
							QTreeWidgetItem* propertyItem = new QTreeWidgetItem;
							propertyItem->setText( 0, propertyName.c_str());
							propertyItem->setData( 0, Qt::UserRole, "property");
							propertyItem->setExpanded(true);
							item->addChild(propertyItem);

							if (propertyType == AnimProperty::Type::Object)
							{
								propertyItem->setData( 1, Qt::UserRole, "property");
								propertyItem->setIcon( 1, QIcon((Engine::instance()->getRootPath() + "engine/modules/anim/editor/icon/add.png").c_str()));
							}
						}
					}
				}
			}
		}
	}

	void TimelinePanel::onSelectProperty()
	{
		QTreeWidgetItem* item = m_treeWidget->currentItem();
		int column = m_treeWidget->currentColumn();
		if (column == 0)
		{
			String userData = item->data(column, Qt::UserRole).toString().toStdString().c_str();
			if (userData == "property")
			{
				TimelinePanelUtil::queryNodePathAndPropertyChain(item, m_currentEditObjectPath, m_currentEditPropertyChain);
			}
			else
			{
				m_currentEditPropertyChain.clear();
			}

			// refresh curve display
			refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);

			// refresh curve key display
			refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		}
	}

	void TimelinePanel::clearCurveItemsTo(int number)
	{
		//for (size_t i = 0; i < m_curveItems.size(); i++)
		//{
		//	if (!m_curveItems[i])
		//	{
		//		vector<Vector2>::type paths;
		//		m_curveItems[i] = qGraphicsSceneAddPath(m_graphicsScene, paths);
		//	}
		//}
	}

	void TimelinePanel::refreshCurveDisplayToEditor(const String& objectPath, const StringArray& propertyChain)
	{
		//// clear curves
		//for (size_t i = 0; i < m_curveItems.size(); i++)
		//{
		//	if (m_curveItems[i])
		//	{
		//		delete m_curveItems[i];
		//		m_curveItems[i] = nullptr;
		//	}
		//}

		//// recreate curves
		//AnimProperty* animProperty = m_timeline->getProperty(m_currentEditAnim, objectPath, propertyChain);
		//if (animProperty)
		//{
		//	switch (animProperty->m_type)
		//	{
		//	case AnimProperty::Type::Bool:
		//		{
		//			Vector2 keyPos;
		//			calcKeyPosByTimeAndValue( animProperty->getLength(), 0.f, keyPos);
		//			m_curveItems[0] = EditorApi.qGraphicsSceneAddLine(m_graphicsScene, 0.f, 0.f, keyPos.x, 0.f, Color(1.f, 0.f, 0.f, 0.7f));
		//		}
		//		break;
		//	case AnimProperty::Type::Vector3:
		//	{
		//		AnimPropertyVec3* vec3Proeprty = ECHO_DOWN_CAST<AnimPropertyVec3*>(animProperty);
		//		if (vec3Proeprty)
		//		{
		//			const ui32 frameStep = 2;

		//			//clearCurveItemsTo(3);
		//			vector<Vector2>::type curvePaths[3];
		//			for (i32 curveIdx = 0; curveIdx < 3; curveIdx++)
		//			{
		//				AnimCurve* curve = vec3Proeprty->m_curves[curveIdx];
		//				if (curve && curve->getKeyCount())
		//				{
		//					for (ui32 t = curve->getStartTime(); t <= curve->getEndTime(); t += frameStep)
		//					{
		//						float value = curve->getValue(t);

		//						Vector2 keyPos;
		//						calcKeyPosByTimeAndValue(t, value, keyPos);
		//						curvePaths[curveIdx].emplace_back(keyPos);
		//					}
		//				}
		//			}

		//			m_curveItems[0] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[0], 2.5f, Color( 1.f, 0.f, 0.f, 0.7f));
		//			m_curveItems[1] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[1], 2.5f, Color( 0.f, 1.f, 0.f, 0.7f));
		//			m_curveItems[2] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[2], 2.5f, Color( 0.f, 0.f, 1.f, 0.7f));
		//		}
		//	}
		//	break;
		//	case AnimProperty::Type::Vector4:
		//	{
		//		AnimPropertyVec4* vec3Proeprty = ECHO_DOWN_CAST<AnimPropertyVec4*>(animProperty);
		//		if (vec3Proeprty)
		//		{
		//			const ui32 frameStep = 2;

		//			//clearCurveItemsTo(3);
		//			vector<Vector2>::type curvePaths[4];
		//			for (i32 curveIdx = 0; curveIdx < 4; curveIdx++)
		//			{
		//				AnimCurve* curve = vec3Proeprty->m_curves[curveIdx];
		//				if (curve && curve->getKeyCount())
		//				{
		//					for (ui32 t = curve->getStartTime(); t <= curve->getEndTime(); t += frameStep)
		//					{
		//						float value = curve->getValue(t);

		//						Vector2 keyPos;
		//						calcKeyPosByTimeAndValue(t, value, keyPos);
		//						curvePaths[curveIdx].emplace_back(keyPos);
		//					}
		//				}
		//			}

		//			m_curveItems[0] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[0], 2.5f, Color(1.f, 0.f, 0.f, 0.7f));
		//			m_curveItems[1] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[1], 2.5f, Color(0.f, 1.f, 0.f, 0.7f));
		//			m_curveItems[2] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[2], 2.5f, Color(0.f, 0.f, 1.f, 0.7f));
		//			m_curveItems[3] = EditorApi.qGraphicsSceneAddPath(m_graphicsScene, curvePaths[3], 2.5f, Color(0.f, 0.f, 1.f, 0.7f));
		//		}
		//	}
		//	break;
  //          default: break;
		//	}
		//}
	}

	void TimelinePanel::refreshCurveKeyDisplayToEditor(const String& objectPath, const StringArray& propertyChain)
	{
		//// clear all key items
		//for (size_t i = 0; i < m_curveItems.size(); i++)
		//{
		//	for (QGraphicsItem* item : m_curveKeyItems[i])
		//	{
		//		m_graphicsScene->removeItem(item);
		//		delete item;
		//	}

		//	m_curveKeyItems[i].clear();
		//}

		//for (QGraphicsProxyWidget* widget : m_curveKeyWidgets)
		//{ 
		//	m_graphicsScene->removeItem(widget);
		//	delete widget;
		//}
		//m_curveKeyWidgets.clear();

		//// create key
		//AnimProperty* animProperty = m_timeline->getProperty(m_currentEditAnim, objectPath, propertyChain);
		//if (animProperty)
		//{
		//	array<Color, 4> KeyColors = { Color::RED, Color::GREEN, Color::BLUE, Color::WHITE};

		//	switch (animProperty->m_type)
		//	{
		//	case AnimProperty::Type::Bool:
		//		{
		//			AnimPropertyBool* boolProperty = ECHO_DOWN_CAST<AnimPropertyBool*>(animProperty);
		//			if (boolProperty)
		//			{
		//				//i32 keyIdx = 0;
		//				for (auto& it : boolProperty->m_keys)
		//				{
		//					ui32 t = it.first;
		//					float value = it.second;

		//					Vector2 center;
		//					calcKeyPosByTimeAndValue(t, value, center);

		//					QWidget* checkBox = new QCheckBox;
		//					QGraphicsProxyWidget* widget = EditorApi.qGraphicsSceneAddWidget(m_graphicsScene, checkBox);

		//					widget->setPos(center.x, 0.f);
		//					widget->setZValue(250.f);

		//					// set userdata
		//					//String userData = StringUtil::Format("%s,%s,%s,%d,%d", m_currentEditAnim.c_str(), objectPath.c_str(), propertyName.c_str(), 0, keyIdx++);
		//					//qGraphicsItemSetUserData(item, userData.c_str());

		//					// set tooltip
		//					//String toolTip = StringUtil::Format("Time : %d\nValue: %.3f", t, value);
		//					//qGraphicsItemSetToolTip(item, toolTip.c_str());

		//					// set moveable
		//					//qGraphicsItemSetMoveable(item, true);

		//					// connect signal slots
		//					//qConnect(item, QSIGNAL(mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)), this, createMethodBind(&TimelinePanel::onKeyDoubleClickedCurveKey));
		//					//qConnect(item, QSIGNAL(mouseMoveEvent(QGraphicsSceneMouseEvent*)), this, createMethodBind(&TimelinePanel::onKeyPositionChanged));

		//					m_curveKeyWidgets.emplace_back( widget);
		//				}
		//			}
		//		}
		//		break;
		//	case AnimProperty::Type::Vector3:
		//	{
		//		AnimPropertyVec3* vec3Proeprty = ECHO_DOWN_CAST<AnimPropertyVec3*>(animProperty);
		//		if (vec3Proeprty)
		//		{
		//			for (int curveIdx = 0; curveIdx < 3; curveIdx++)
		//			{
		//				AnimCurve* curve = vec3Proeprty->m_curves[curveIdx];
		//				for (int keyIdx = 0; keyIdx < curve->getKeyCount(); keyIdx++)
		//				{
		//					ui32 t = curve->getKeyTime(keyIdx);
		//					float value = curve->getValueByKeyIdx(keyIdx);

		//					Vector2 center;
		//					calcKeyPosByTimeAndValue(t, value, center);

		//					QGraphicsItem* item = EditorApi.qGraphicsSceneAddEclipse(m_graphicsScene, center.x - m_keyRadius, center.y - m_keyRadius, m_keyRadius * 2.f, m_keyRadius*2.f, KeyColors[curveIdx]);

		//					// set user data
		//					String userData = StringUtil::Format("%s,%s,%s,%d,%d", m_currentEditAnim.c_str(), objectPath.c_str(), StringUtil::ToString(propertyChain).c_str(), curveIdx, keyIdx);
		//					item->setData(Qt::UserRole, userData.c_str());

		//					// set tooltip
		//					String toolTip = StringUtil::Format("Time : %d\nValue: %.3f", t, value);
		//					item->setToolTip(toolTip.c_str());

		//					// set move able
		//					item->setFlag(QGraphicsItem::ItemIsMovable, true);

		//					// connect signal slots
		//					EditorApi.qConnectGraphicsItem(item, QSIGNAL(mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)), this, createMethodBind(&TimelinePanel::onKeyDoubleClickedCurveKey));
		//					EditorApi.qConnectGraphicsItem(item, QSIGNAL(mouseMoveEvent(QGraphicsSceneMouseEvent*)), this, createMethodBind(&TimelinePanel::onKeyPositionChanged));

		//					m_curveKeyItems[curveIdx].emplace_back(item);
		//				}
		//			}
		//		}
		//	}
		//	break;
		//	case AnimProperty::Type::String:
		//	{
		//		AnimPropertyString* strProperty = ECHO_DOWN_CAST<AnimPropertyString*>(animProperty);
		//		if (strProperty)
		//		{
		//			//i32 keyIdx = 0;
		//			for (auto& it : strProperty->m_keys)
		//			{
		//				ui32 t = it.first;
		//				String value = it.second;

		//				Vector2 center;
		//				calcKeyPosByTimeAndValue(t, 0.f, center);

		//				QWidget* checkBox = new QCheckBox;
		//				QGraphicsProxyWidget* widget = EditorApi.qGraphicsSceneAddWidget(m_graphicsScene, checkBox);

		//				widget->setPos(center.x, 50.f);
		//				widget->setZValue(250.f);

		//				m_curveKeyWidgets.emplace_back(widget);
		//			}
		//		}
		//	}
		//	break;
  //                  
  //          default: break;
		//	}
		//}
	}

	bool TimelinePanel::getKeyInfo(TimelinePanel::KeyInfo& keyInfo, const String& animName, const String& objectPath, const String& propertyName, int curveIdx, int keyIdx)
	{
		AnimProperty* animProperty = m_timeline->getProperty(m_currentEditAnim, objectPath, StringUtil::Split(propertyName));
		if (animProperty)
		{
			switch (animProperty->m_type)
			{
			case AnimProperty::Type::Vector3:
			{
				AnimPropertyVec3* vec3Property = ECHO_DOWN_CAST<AnimPropertyVec3*>(animProperty);
				if (vec3Property)
				{
					AnimCurve* curve = vec3Property->m_curves[curveIdx];

					keyInfo.m_type = KeyInfo::Type::Float;
					keyInfo.m_value = curve->getValueByKeyIdx(keyIdx);

					return true;
				}
			}
			break;
                    
            default: break;
			}
		}

		return false;
	}

	void TimelinePanel::onKeyDoubleClickedCurveKey()
	{
		//// show key value editor widget
		//if (!m_curveKeyLineEdit)
		//{
		//	m_curveKeyLineEdit = new QLineEdit;
		//	m_curveKeyLineEdit->setMaximumWidth(100);
		//	m_curveKeyLineEditProxyWidget = EditorApi.qGraphicsSceneAddWidget(m_graphicsScene, m_curveKeyLineEdit);

		//	EditorApi.qConnectWidget(m_curveKeyLineEdit, QSIGNAL(editingFinished()), this, createMethodBind(&TimelinePanel::onCurveKeyEditingFinished));
		//}

		//// get current key value
		//QGraphicsItem* sender = EditorApi.qSenderItem();
		//if (sender)
		//{
		//	String userData = sender->data(Qt::UserRole).toString().toStdString().c_str();
		//	StringArray userDataSplits = StringUtil::Split(userData, ",");
		//	KeyInfo keyInfo;
		//	if (getKeyInfo(keyInfo, userDataSplits[0], userDataSplits[1], userDataSplits[2], StringUtil::ParseI32(userDataSplits[3]), StringUtil::ParseI32(userDataSplits[4])))
		//	{
		//		m_curveKeyItem = sender;
		//		m_curveKeyLineEdit->setText( StringUtil::ToString( keyInfo.m_value).c_str());

		//		int halfHeight = m_curveKeyLineEdit->height() / 2;
		//		Vector2 mouseScenePos = EditorApi.qGraphicsItemGetEventAll(sender).graphicsSceneMouseEvent.scenePos;
		//		m_curveKeyLineEditProxyWidget->setPos(mouseScenePos.x + m_keyRadius, mouseScenePos.y - halfHeight);
		//		m_curveKeyLineEditProxyWidget->setZValue(250.f);
		//	}
		//}

		//m_curveKeyLineEdit->setVisible(true);
		//m_curveKeyLineEdit->setCursorPosition(0);
	}

	void TimelinePanel::onCurveKeyEditingFinished()
	{
		//String valueStr = m_curveKeyLineEdit->text().toStdString().c_str();
		//if (!valueStr.empty() && m_curveKeyItem)
		//{
		//	String userData = m_curveKeyItem->data(Qt::UserRole).toString().toStdString().c_str();
		//	StringArray userDataSplits = StringUtil::Split(userData, ",");

		//	// modify key value
		//	float value = StringUtil::ParseFloat(valueStr);
		//	m_timeline->setKey(userDataSplits[0], userDataSplits[1], userDataSplits[2], StringUtil::ParseI32(userDataSplits[3]), StringUtil::ParseI32(userDataSplits[4]), value);
		//}

		//m_curveKeyLineEdit->setVisible(false);

		//// refresh curve and key display
		//refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		//refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
	}

	void TimelinePanel::onKeyPositionChanged()
	{
		QGraphicsItem* sender = EditorApi.qSenderItem();
		if (sender)
		{
			Vector2 scenePos(sender->pos().x(), sender->pos().y());
			i32 time;
			float value;
			calcKeyTimeAndValueByPos(scenePos, time, value);

			// modify tooltip
			String toolTip = StringUtil::Format("Time : %d\nValue: %.3f", time, value);
			sender->setToolTip(toolTip.c_str());
		}
	}

	void TimelinePanel::onCurrentEditAnimChanged()
	{
		String currentText = m_clipsComboBox->currentText().toStdString().c_str();
		setCurrentEditAnim( currentText.c_str());

		m_treeWidgetHeader->getToolBar()->onStopAnim();
	}

	void TimelinePanel::onCurrentEditAnimLengthChanged()
	{
		if (!m_currentEditAnim.empty())
		{
			AnimClip* clip = m_timeline->getClip(m_currentEditAnim.c_str());
			if (clip)
			{
				ui32 newLength = StringUtil::ParseUI32(m_clipLengthLineEdit->text().toStdString().c_str(), clip->m_length);
				clip->setLength(newLength);
			}
		}
	}

	void TimelinePanel::onSwitchCurveVisibility()
	{
		//m_curveVisibles[0] = m_curveXVisible->isChecked();
		//m_curveVisibles[1] = m_curveYVisible->isChecked();
		//m_curveVisibles[2] = m_curveZVisible->isChecked();
		//m_curveVisibles[3] = m_curveWVisible->isChecked();

		//for (size_t i = 0; i < m_curveItems.size(); i++)
		//{
		//	if (m_curveItems[i])
		//	{
		//		m_curveItems[i]->setVisible(m_curveVisibles[i]);
		//	}

		//	for (QGraphicsItem* keyItem : m_curveKeyItems[i])
		//	{
		//		keyItem->setVisible(m_curveVisibles[i]);
		//	}
		//}
	}

	void TimelinePanel::onRightClickGraphicsView()
	{
		// clear menu
		if (m_keyEditMenu)
		{
			EchoSafeDelete(m_keyEditMenu, QMenu);
		}

		AnimProperty* animProperty = m_timeline->getProperty(m_currentEditAnim, m_currentEditObjectPath, m_currentEditPropertyChain);
		if (animProperty)
		{
			if (animProperty->getType() == AnimProperty::Type::Bool)
			{
				m_keyEditMenu = new QMenu(this);

				m_keyEditMenu->addAction(m_actionAddBoolKeyToCurve);
				EditorApi.qConnectAction(m_actionAddBoolKeyToCurve, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddBoolKeyToCurve));
			}
			else if (animProperty->getType() == AnimProperty::Type::Vector3)
			{
				m_keyEditMenu = new QMenu(this);

				m_keyEditMenu->addAction(m_actionAddKeyToCurveRed);
				m_keyEditMenu->addAction(m_actionAddKeyToCurveGreen);
				m_keyEditMenu->addAction(m_actionAddKeyToCurveBlue);
				EditorApi.qConnectAction(m_actionAddKeyToCurveRed, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddKeyToCurveRed));
				EditorApi.qConnectAction(m_actionAddKeyToCurveGreen, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddKeyToCurveGreen));
				EditorApi.qConnectAction(m_actionAddKeyToCurveBlue, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddKeyToCurveBlue));
			}
			else if (animProperty->getType() == AnimProperty::Type::String)
			{
				m_keyEditMenu = new QMenu(this);

				m_keyEditMenu->addAction(m_actionAddStringKeyToCurve);
				EditorApi.qConnectAction(m_actionAddStringKeyToCurve, QSIGNAL(triggered()), this, createMethodBind(&TimelinePanel::onAddStringKeyToCurve));
			}
		}

		// record cursor pos
		//m_keyEditCursorScenePos = EditorApi.qObjectGetEventAll(m_graphicsScene).graphicsSceneMouseEvent.scenePos;

		// show menu
		m_keyEditMenu->exec(QCursor::pos());
	}

	void TimelinePanel::onGraphicsSceneWheelEvent()
	{
		//int delta = EditorApi.qObjectGetEventAll(m_graphicsScene).graphicsSceneWheelEvent.delta;
		//
		//m_millisecondPerPixel += delta > 0 ? -0.05f : 0.05f;
		//m_millisecondPerPixel = Math::Clamp(m_millisecondPerPixel, 1.f, 50.f);

		//m_unitsPerPixel += delta > 0 ? -0.05f : 0.05f;
		//m_unitsPerPixel = Math::Clamp(m_unitsPerPixel, 1.f, 50.f);

		//// refresh curve and key display
		//refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		//refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
	}

	bool TimelinePanel::calcKeyTimeAndValueByPos(const Vector2& pos, i32& time, float& value)
	{
		//time = ui32(pos.x * m_millisecondPerPixel);
		//value = (pos.y) * m_unitsPerPixel;

		return true;
	}

	bool TimelinePanel::calcKeyPosByTimeAndValue(i32 time, float value, Vector2& pos)
	{
		//pos = Vector2(time / m_millisecondPerPixel, value / m_unitsPerPixel);

		return true;
	}

	void TimelinePanel::onAddBoolKeyToCurve()
	{
		i32 time;
		float value;
		if (calcKeyTimeAndValueByPos(m_keyEditCursorScenePos, time, value))
		{
			time = Math::Clamp(time, 0, 1000 * 60 * 60 * 24);
			m_timeline->addKey(m_currentEditAnim, m_currentEditObjectPath, StringUtil::ToString(m_currentEditPropertyChain), time, true);
		}

		// refresh curve and key display
		refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
	}

	void TimelinePanel::onAddStringKeyToCurve()
	{
		i32 time;
		float value;
		if (calcKeyTimeAndValueByPos(m_keyEditCursorScenePos, time, value))
		{
			time = Math::Clamp(time, 0, 1000 * 60 * 60 * 24);

			Variant::Type variantType = m_timeline->getAnimPropertyVariableType(m_currentEditObjectPath, m_currentEditPropertyChain);
			if (variantType == Variant::Type::ResourcePath)
			{
				Variant currentValue;
				if (TimelinePanelUtil::getProertyValue(m_timeline, m_currentEditObjectPath, m_currentEditPropertyChain, currentValue))
				{
					String exts = currentValue.toResPath().getSupportExts().c_str();
					String path = Editor::instance()->selectAResObject(exts.c_str());
					m_timeline->addKey(m_currentEditAnim, m_currentEditObjectPath, StringUtil::ToString(m_currentEditPropertyChain), time, path);
				}
			}
		}

		// refresh curve and key display
		refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
	}

	void TimelinePanel::onAddKeyToCurveRed()
	{
		addKeyToCurve(0);
	}

	void TimelinePanel::onAddKeyToCurveGreen()
	{
		addKeyToCurve(1);
	}

	void TimelinePanel::onAddKeyToCurveBlue()
	{
		addKeyToCurve(2);
	}

	void TimelinePanel::onAddKeyToCurveWhite()
	{
		addKeyToCurve(3);
	}

	void TimelinePanel::addKeyToCurve(int curveIdx)
	{
		i32 time;
		float value;
		if (calcKeyTimeAndValueByPos(m_keyEditCursorScenePos, time, value))
		{
			time = Math::Clamp(time, 0, 1000 * 60 * 60 * 24);
			m_timeline->addKey(m_currentEditAnim, m_currentEditObjectPath, StringUtil::ToString(m_currentEditPropertyChain), curveIdx, time, value);
		}

		// refresh curve and key display
		refreshCurveDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
		refreshCurveKeyDisplayToEditor(m_currentEditObjectPath, m_currentEditPropertyChain);
	}
}

#endif

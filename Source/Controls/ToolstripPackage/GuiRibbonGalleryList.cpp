#include "GuiRibbonGalleryList.h"
#include "GuiRibbonImpl.h"
#include "../Templates/GuiThemeStyleFactory.h"

/* CodePack:BeginIgnore() */
#ifndef VCZH_DEBUG_NO_REFLECTION
/* CodePack:ConditionOff(VCZH_DEBUG_NO_REFLECTION, ../Reflection/TypeDescriptors/GuiReflectionPlugin.h) */
#include "../../Reflection/TypeDescriptors/GuiReflectionPlugin.h"
#endif
/* CodePack:EndIgnore() */

namespace vl
{
	namespace presentation
	{
		namespace controls
		{
			using namespace reflection::description;
			using namespace collections;
			using namespace compositions;
			using namespace templates;

			namespace list
			{

/***********************************************************************
list::GalleryGroup
***********************************************************************/

				GalleryGroup::GalleryGroup()
				{
				}

				GalleryGroup::~GalleryGroup()
				{
					if (eventHandler)
					{
						itemValues.Cast<IValueObservableList>()->ItemChanged.Remove(eventHandler);
					}
				}

				WString GalleryGroup::GetName()
				{
					return name;
				}

				Ptr<IValueList> GalleryGroup::GetItemValues()
				{
					return itemValues;
				}

/***********************************************************************
list::GroupedDataSource
***********************************************************************/

				void GroupedDataSource::RebuildItemSource()
				{
					ignoreGroupChanged = true;
					joinedItemSource.Clear();
					groupedItemSource.Clear();
					cachedGroupItemCounts.Clear();
					ignoreGroupChanged = false;

					if (itemSource)
					{
						if (GetGroupEnabled())
						{
							FOREACH_INDEXER(Value, groupValue, index, GetLazyList<Value>(itemSource))
							{
								auto group = MakePtr<GalleryGroup>();
								group->name = titleProperty(groupValue);
								group->itemValues = GetChildren(childrenProperty(groupValue));
								AttachGroupChanged(group, index);
								groupedItemSource.Add(group);
							}
						}
						else
						{
							auto group = MakePtr<GalleryGroup>();
							group->itemValues = GetChildren(itemSource);
							AttachGroupChanged(group, 0);
							groupedItemSource.Add(group);
						}
					}
				}

				Ptr<IValueList> GroupedDataSource::GetChildren(Ptr<IValueEnumerable> children)
				{
					if (!children)
					{
						return nullptr;
					}
					else if (auto list = children.Cast<IValueList>())
					{
						return list;
					}
					else
					{
						return IValueList::Create(GetLazyList<Value>(children));
					}
				}

				void GroupedDataSource::AttachGroupChanged(Ptr<GalleryGroup> group, vint index)
				{
					if (auto observable = group->itemValues.Cast<IValueObservableList>())
					{
						group->eventHandler = observable->ItemChanged.Add([this, index](vint start, vint oldCount, vint newCount)
						{
							OnGroupItemChanged(index, start, oldCount, newCount);
						});
					}
				}

				void GroupedDataSource::OnGroupChanged(vint start, vint oldCount, vint newCount)
				{
					if (!ignoreGroupChanged)
					{
						for (vint i = 0; i < oldCount; i++)
						{
							RemoveGroupFromJoined(start);
						}
						for (vint i = 0; i < newCount; i++)
						{
							InsertGroupToJoined(start + i);
						}
					}
				}

				void GroupedDataSource::OnGroupItemChanged(vint index, vint start, vint oldCount, vint newCount)
				{
					if (!ignoreGroupChanged)
					{
						vint countBeforeGroup = GetCountBeforeGroup(index);
						vint joinedIndex = countBeforeGroup + start;
						vint minCount = oldCount < newCount ? oldCount : newCount;
						auto itemValues = groupedItemSource[index]->itemValues;

						for (vint i = 0; i < minCount; i++)
						{
							joinedItemSource.Set(joinedIndex + i, itemValues->Get(start + i));
						}

						if (oldCount < newCount)
						{
							for (vint i = minCount; i < newCount; i++)
							{
								joinedItemSource.Insert(joinedIndex + i, itemValues->Get(start + i));
							}
						}
						else if (oldCount > newCount)
						{
							for (vint i = minCount; i < oldCount; i++)
							{
								joinedItemSource.RemoveAt(joinedIndex + i);
							}
						}

						cachedGroupItemCounts[index] += newCount - oldCount;
					}
				}

				vint GroupedDataSource::GetCountBeforeGroup(vint index)
				{
					vint count = 0;
					for (vint i = 0; i < index; i++)
					{
						count += cachedGroupItemCounts[i];
					}
					return count;
				}

				void GroupedDataSource::InsertGroupToJoined(vint index)
				{
					vint countBeforeGroup = GetCountBeforeGroup(index);
					auto group = groupedItemSource[index];
					vint itemCount = group->itemValues ? group->itemValues->GetCount() : 0;
					cachedGroupItemCounts.Insert(index, itemCount);

					if (itemCount > 0)
					{
						for (vint i = 0; i < itemCount; i++)
						{
							joinedItemSource.Insert(countBeforeGroup + i, group->itemValues->Get(i));
						}
					}
				}

				void GroupedDataSource::RemoveGroupFromJoined(vint index)
				{
					vint countBeforeGroup = GetCountBeforeGroup(index);
					joinedItemSource.RemoveRange(countBeforeGroup, cachedGroupItemCounts[index]);
					cachedGroupItemCounts.RemoveAt(index);
				}

				GroupedDataSource::GroupedDataSource(compositions::GuiGraphicsComposition* _associatedComposition)
					:associatedComposition(_associatedComposition)
				{
					GroupEnabledChanged.SetAssociatedComposition(associatedComposition);
					GroupTitlePropertyChanged.SetAssociatedComposition(associatedComposition);
					GroupChildrenPropertyChanged.SetAssociatedComposition(associatedComposition);

					groupChangedHandler = groupedItemSource.GetWrapper()->ItemChanged.Add(this, &GroupedDataSource::OnGroupChanged);
				}

				GroupedDataSource::~GroupedDataSource()
				{
					joinedItemSource.GetWrapper()->ItemChanged.Remove(groupChangedHandler);
				}

				Ptr<IValueEnumerable> GroupedDataSource::GetItemSource()
				{
					return itemSource;
				}

				void GroupedDataSource::SetItemSource(Ptr<IValueEnumerable> value)
				{
					if (itemSource != value)
					{
						itemSource = value;
						RebuildItemSource();
					}
				}

				bool GroupedDataSource::GetGroupEnabled()
				{
					return titleProperty && childrenProperty;
				}

				ItemProperty<WString> GroupedDataSource::GetGroupTitleProperty()
				{
					return titleProperty;
				}

				void GroupedDataSource::SetGroupTitleProperty(const ItemProperty<WString>& value)
				{
					if (titleProperty != value)
					{
						titleProperty = value;
						GroupTitlePropertyChanged.Execute(GuiEventArgs(associatedComposition));
						GroupEnabledChanged.Execute(GuiEventArgs(associatedComposition));
						RebuildItemSource();
					}
				}

				ItemProperty<Ptr<IValueEnumerable>> GroupedDataSource::GetGroupChildrenProperty()
				{
					return childrenProperty;
				}

				void GroupedDataSource::SetGroupChildrenProperty(const ItemProperty<Ptr<IValueEnumerable>>& value)
				{
					if (childrenProperty != value)
					{
						childrenProperty = value;
						GroupChildrenPropertyChanged.Execute(GuiEventArgs(associatedComposition));
						GroupEnabledChanged.Execute(GuiEventArgs(associatedComposition));
						RebuildItemSource();
					}
				}

				const GroupedDataSource::GalleryGroupList& GroupedDataSource::GetGroups()
				{
					return groupedItemSource;
				}
			}

/***********************************************************************
GuiBindableRibbonGalleryList
***********************************************************************/

			void GuiBindableRibbonGalleryList::BeforeControlTemplateUninstalled_()
			{
			}

			void GuiBindableRibbonGalleryList::AfterControlTemplateInstalled_(bool initialize)
			{
				auto ct = GetControlTemplateObject();
				itemList->SetControlTemplate(ct->GetItemListTemplate());
				subMenu->SetControlTemplate(ct->GetMenuTemplate());
				groupContainer->SetControlTemplate(ct->GetGroupContainerTemplate());
				MenuResetGroupTemplate();
				UpdateLayoutSizeOffset();
			}

			void GuiBindableRibbonGalleryList::UpdateLayoutSizeOffset()
			{
				auto cSize = itemList->GetContainerComposition()->GetBounds();
				auto bSize = itemList->GetBoundsComposition()->GetBounds();
				layout->SetSizeOffset(Size(bSize.Width() - cSize.Width(), bSize.Height() - cSize.Height()));
			}

			void GuiBindableRibbonGalleryList::OnBoundsChanged(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				UpdateLayoutSizeOffset();
				subMenu->GetBoundsComposition()->SetPreferredMinSize(Size(boundsComposition->GetBounds().Width(), 1));
			}

			void GuiBindableRibbonGalleryList::OnRequestedDropdown(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				subMenu->ShowPopup(this, Point(0, 0));
			}

			void GuiBindableRibbonGalleryList::OnRequestedScrollUp(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				itemListArranger->ScrollUp();
			}

			void GuiBindableRibbonGalleryList::OnRequestedScrollDown(compositions::GuiGraphicsComposition* sender, compositions::GuiEventArgs& arguments)
			{
				itemListArranger->ScrollDown();
			}

			void GuiBindableRibbonGalleryList::MenuResetGroupTemplate()
			{
				groupStack->SetItemTemplate([this](const Value& groupValue)->GuiTemplate*
				{
					auto group = UnboxValue<Ptr<list::GalleryGroup>>(groupValue);

					auto groupTemplate = new GuiTemplate;
					groupTemplate->SetMinSizeLimitation(GuiGraphicsComposition::LimitToElementAndChildren);

					auto groupContentStack = new GuiStackComposition;
					groupContentStack->SetMinSizeLimitation(GuiGraphicsComposition::LimitToElementAndChildren);
					groupContentStack->SetAlignmentToParent(Margin(0, 0, 0, 0));
					groupContentStack->SetDirection(GuiStackComposition::Vertical);
					{
						auto item = new GuiStackItemComposition;
						groupContentStack->AddChild(item);

						auto header = new GuiControl(theme::ThemeName::RibbonToolstripHeader);
						header->SetControlTemplate(GetControlTemplateObject()->GetHeaderTemplate());
						header->GetBoundsComposition()->SetAlignmentToParent(Margin(0, 0, 0, 0));
						header->SetText(group->GetName());
						item->AddChild(header->GetBoundsComposition());
					}
					if (itemStyle)
					{
						auto item = new GuiStackItemComposition;
						item->SetPreferredMinSize(Size(1, 1));
						groupContentStack->AddChild(item);

						auto groupItemFlow = new GuiRepeatFlowComposition();
						groupItemFlow->SetMinSizeLimitation(GuiGraphicsComposition::LimitToElementAndChildren);
						groupItemFlow->SetAlignmentToParent(Margin(0, 0, 0, 0));
						groupItemFlow->SetItemSource(group->GetItemValues());
						groupItemFlow->SetItemTemplate([this](const Value& groupItemValue)->GuiTemplate*
						{
							auto groupItemTemplate = new GuiTemplate;
							groupItemTemplate->SetMinSizeLimitation(GuiGraphicsComposition::LimitToElementAndChildren);

							auto backgroundButton = new GuiSelectableButton(theme::ThemeName::ListItemBackground);
							if (auto style = GetControlTemplateObject()->GetBackgroundTemplate())
							{
								backgroundButton->SetControlTemplate(style);
							}
							backgroundButton->SetAutoSelection(false);
							groupItemTemplate->AddChild(backgroundButton->GetBoundsComposition());

							auto itemTemplate = itemStyle(groupItemValue);
							itemTemplate->SetAlignmentToParent(Margin(0, 0, 0, 0));
							backgroundButton->GetContainerComposition()->AddChild(itemTemplate);

							return groupItemTemplate;
						});
						item->AddChild(groupItemFlow);
					}
					groupTemplate->AddChild(groupContentStack);

					return groupTemplate;
				});
			}

			GuiControl* GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint groupIndex)
			{
				CHECK_ERROR(0 <= groupIndex && groupIndex < groupedItemSource.Count(), L"GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint)#Group index out of range");
				auto stackItem = groupStack->GetStackItems()[groupIndex];
				auto groupTemplate = stackItem->Children()[0];
				auto groupContentStack = dynamic_cast<GuiStackComposition*>(groupTemplate->Children()[0]);
				auto groupHeaderItem = groupContentStack->GetStackItems()[0];
				auto groupHeader = groupHeaderItem->Children()[0]->GetAssociatedControl();
				CHECK_ERROR(groupHeader, L"GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint)#Internal error.");
				return groupHeader;
			}

			compositions::GuiRepeatFlowComposition* GuiBindableRibbonGalleryList::MenuGetGroupFlow(vint groupIndex)
			{
				CHECK_ERROR(0 <= groupIndex && groupIndex < groupedItemSource.Count(), L"GuiBindableRibbonGalleryList::MenuGetGroupFlow(vint)#Group index out of range");
				if (!itemStyle) return nullptr;
				auto stackItem = groupStack->GetStackItems()[groupIndex];
				auto groupTemplate = stackItem->Children()[0];
				auto groupContentStack = dynamic_cast<GuiStackComposition*>(groupTemplate->Children()[0]);
				auto groupContentItem = groupContentStack->GetStackItems()[1];
				auto groupFlow = dynamic_cast<GuiRepeatFlowComposition*>(groupContentItem->Children()[0]);
				CHECK_ERROR(groupFlow, L"GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint)#Internal error.");
				return groupFlow;
			}

			GuiSelectableButton* GuiBindableRibbonGalleryList::MenuGetGroupItemBackground(vint groupIndex, vint itemIndex)
			{
				CHECK_ERROR(0 <= groupIndex && groupIndex < groupedItemSource.Count(), L"GuiBindableRibbonGalleryList::MenuGetGroupItemBackground(vint, vint)#Group index out of range");
				auto group = groupedItemSource[groupIndex];
				CHECK_ERROR(group->GetItemValues() && 0 <= itemIndex && itemIndex < group->GetItemValues()->GetCount(), L"GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint, vint)#Item index out of range");

				auto groupFlow = MenuGetGroupFlow(groupIndex);
				auto groupFlowItem = groupFlow->GetFlowItems()[itemIndex];
				auto groupItemTemplate = groupFlowItem->Children()[0];
				auto groupItemBackground = dynamic_cast<GuiSelectableButton*>(groupItemTemplate->Children()[0]->GetAssociatedControl());
				CHECK_ERROR(groupItemBackground, L"GuiBindableRibbonGalleryList::MenuGetGroupHeader(vint, vint)#Internal error.");
				return groupItemBackground;
			}

			GuiBindableRibbonGalleryList::GuiBindableRibbonGalleryList(theme::ThemeName themeName)
				:GuiRibbonGallery(themeName)
				, GroupedDataSource(boundsComposition)
			{
				ItemTemplateChanged.SetAssociatedComposition(boundsComposition);
				SelectionChanged.SetAssociatedComposition(boundsComposition);
				subMenu = new GuiRibbonToolstripMenu(theme::ThemeName::RibbonToolstripMenu, this);

				{
					layout = new ribbon_impl::GalleryResponsiveLayout;
					layout->SetAlignmentToParent(Margin(0, 0, 0, 0));
					containerComposition->AddChild(layout);

					itemListArranger = new ribbon_impl::GalleryItemArranger(this);
					itemList = new GuiBindableTextList(theme::ThemeName::RibbonGalleryItemList);
					itemList->GetBoundsComposition()->SetAlignmentToParent(Margin(0, 0, 0, 0));
					itemList->SetArranger(itemListArranger);
					itemList->SetItemSource(joinedItemSource.GetWrapper());
					layout->AddChild(itemList->GetBoundsComposition());
				}
				{
					groupContainer = new GuiScrollContainer(theme::ThemeName::ScrollView);
					groupContainer->SetHorizontalAlwaysVisible(false);
					groupContainer->SetVerticalAlwaysVisible(false);
					groupContainer->SetExtendToFullWidth(true);
					groupContainer->GetBoundsComposition()->SetAlignmentToParent(Margin(0, 0, 0, 0));
					subMenu->GetContentComposition()->AddChild(groupContainer->GetBoundsComposition());

					groupStack = new GuiRepeatStackComposition();
					groupStack->SetMinSizeLimitation(GuiGraphicsComposition::LimitToElementAndChildren);
					groupStack->SetAlignmentToParent(Margin(0, 0, 0, 0));
					groupStack->SetDirection(GuiStackComposition::Vertical);
					groupStack->SetItemSource(groupedItemSource.GetWrapper());
					groupContainer->GetContainerComposition()->AddChild(groupStack);
					MenuResetGroupTemplate();
				}

				RequestedScrollUp.AttachMethod(this, &GuiBindableRibbonGalleryList::OnRequestedScrollUp);
				RequestedScrollDown.AttachMethod(this, &GuiBindableRibbonGalleryList::OnRequestedScrollDown);
				RequestedDropdown.AttachMethod(this, &GuiBindableRibbonGalleryList::OnRequestedDropdown);
				boundsComposition->BoundsChanged.AttachMethod(this, &GuiBindableRibbonGalleryList::OnBoundsChanged);
				itemListArranger->UnblockScrollUpdate();
			}

			GuiBindableRibbonGalleryList::~GuiBindableRibbonGalleryList()
			{
				delete subMenu;
			}

			GuiBindableRibbonGalleryList::ItemStyleProperty GuiBindableRibbonGalleryList::GetItemTemplate()
			{
				return itemStyle;
			}

			void GuiBindableRibbonGalleryList::SetItemTemplate(ItemStyleProperty value)
			{
				if (itemStyle != value)
				{
					itemStyle = value;
					itemList->SetItemTemplate(value);
					ItemTemplateChanged.Execute(GetNotifyEventArguments());
				}
			}

			vint GuiBindableRibbonGalleryList::GetMinCount()
			{
				return layout->GetMinCount();
			}

			void GuiBindableRibbonGalleryList::SetMinCount(vint value)
			{
				layout->SetMinCount(value);
			}

			vint GuiBindableRibbonGalleryList::GetMaxCount()
			{
				return layout->GetMaxCount();
			}

			void GuiBindableRibbonGalleryList::SetMaxCount(vint value)
			{
				layout->SetMaxCount(value);
			}

			GalleryPos GuiBindableRibbonGalleryList::GetSelection()
			{
				throw 0;
			}

			void GuiBindableRibbonGalleryList::SetSelection(GalleryPos value)
			{
				throw 0;
			}

			GuiToolstripMenu* GuiBindableRibbonGalleryList::GetSubMenu()
			{
				return subMenu;
			}
		}
	}
}
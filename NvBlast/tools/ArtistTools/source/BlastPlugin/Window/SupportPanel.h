#ifndef SUPPORTPANEL_H
#define SUPPORTPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class SupportPanel;
}

class SupportPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SupportPanel(QWidget *parent = 0);
    ~SupportPanel();
	void updateValues();

private slots:
    void on_comboBoxHealthMask_currentIndexChanged(int index);

    void on_btnAddHealthMask_clicked();

    void on_btnPen_clicked();

    void on_btnRemove_clicked();

    void on_spinBoxBondStrength_valueChanged(double arg1);

    void on_checkBoxEnableJoint_stateChanged(int arg1);

private:
    Ui::SupportPanel *ui;
};

#endif // SUPPORTPANEL_H

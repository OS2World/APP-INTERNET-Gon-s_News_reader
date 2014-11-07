gn の著作権などについて

著作権について

client.c, nntp.h は nntp 1.5.11 に附属のファイルを一部改変して使用して
います。著作権、配布条件等は nntp 1.5.11 オリジナルに準じます。
client.c の先頭部分にある表示を参照下さい。

nntp.c auth.c,smtp.c は client.c をベースに改変したものです。
著作権、配布条件等は client.c に準じます。
＃もうオリジナルの影を残すのはコメントだけとなってしまった。

getopt.c は 4.3BSD のものです。著作権、配布条件等は先頭部分にある表示
を参照下さい。

jnames.c は xklock とともに、fj.sources にポストされたものを改変したも
のです。

mkfiles/unix/old/mvux, docs.jp/unix/mvux
および client.c の #ifdef MVUX 部は
佐賀大学の渡辺健次（watanabe@cc.saga-u.ac.jp）どの、
和歌山大学の床井浩平（kohe-t@eco.wakayama-u.ac.jp）どの
によるものです。

mkfiles/unix/pc-uxv および init.c の #ifdef NECODT 部は
神戸日本電気ソフトウェアの佃（kazu@oa2.kb.nec.co.jp）どの
によるものです。

各モードの、検索コマンドは
三洋電機の土屋（seichi@seichi.cmpd.sanyo.co.jp）どの
によるものです。

inetbios.c inetbios.h は
ムライ電気工業(株)の沢田(sawada@murai-electric.co.jp)どのによるもので
す。先頭付近にある表示を参照下さい。
その他にも、#ifdef MSDOS 、#ifdef INETBIOS、#ifdef GNUDOS の部分には、
沢田どのによるものが多く含まれます。

MS-C 5.0 に附属する MAKE.EXE でコンパイルするための
mkfiles/dos/old/inetbios.ms5 は
国立生理学研究所電子計算機室の水谷文保(mizutani@nips.ac.jp)どのの手に
よるものです。

PC-NFS toolkit ver.4.0 への対応は
東芝 (S研) 小山 徳章(nori@ssel.toshiba.co.jp)どのによるものです。

リコー STARLAN TCP/IP (PathWay for DOS)、NOVELL LAN WorkPlace for DOS
への対応はリコーの小林秀樹(koba@bss.ts.ricoh.co.jp)どのによるものです。

fgets.c および #ifdef USE_FGETS 部のうち SYSV_TERMIO 部以外、
mkfiles/* の fgets 部、Turbo-C への対応は
井上＠ＮＨＫ技研(inouet@strl.nhk.or.jp) どのによるものです。

jnames ライブラリ対応（#ifdef JNAMES 部）のうち、
アーティクルモード、フォロー時に関しては、
都立大学の西川 (kiyoshi@isys3.eei.metro-u.ac.jp)どのによるもの、
ニュースグループモードに関しては
しもだ＠EPD.OMRON(shimoda@psg.krc.omron.co.jp)どのによるものです。

rn ライクな引用タグ生成機能、およびリプライ時の Cc:/Bcc: 機能は
黒江＠大日本印刷(a_kuroe@cdc.dnp.co.jp)どのによるものです。

HP-UX 対応は、
市川＠コンピュータ・アプリケーション（ichi@ca-server.comp-appl.co.jp）
どのによるものです。

OS/2 対応は、日本 IBM の菊池（kiku@yamato.ibm.co.jp）どのによるものです。

setlocale 、 NEWS-OS 6.0 (SVR4) 対応は、
Hideo Irie <michael@sm.sony.co.jp> どのによるものです。

OSF/1 対応は日本ＤＥＣ 国際システム開発部の
橋本 芳昭 （hasimoto@jrd.dec-j.co.jp）どのによるものです。

X680x0 + Human68k への対応は、オムロン ソフトウェアの
阪長（sakanaga@pln.kyoto.omronsoft.co.jp）によるものです。

ME/UX 対応は三菱電機の垣谷（kakiya@han.cow.melco.co.jp）どのによるもの
です。

Date: ヘッダの JST での表示機能は、オムロン株式会社 EFTS統轄事業部の
恵畑 俊彦（ebata@eftses.krc.omron.co.jp）どのによるものです。

EWS4800 (RISC) への対応は NEC コンピュータ事業部 の太田俊哉
(oota@pes.com1.fc.nec.co.jp)どのによるものです。

PC/TCP,SLIM/TCP への対応は、山崎＠九大航空
（yamasaki@aero.kyushu-u.ac.jp）どのによるものです。

X680x0 + Human68k + Ethernet Starter Pack for X680x0 (ESPX) への対応は
野首（knok@da2.so-net.or.jp）どのによるものです。

MH と gnmail/gnspool を組合わせてメイルを送るためのパッチ／ドキュメン
トはひばら＠ソネット（yasasii@ca2.so-net.or.jp）どのによるものです。

docs/readme ははらだ＠神戸（kiroh@kh.rim.or.jp）どのによるものです。

Win32 対応は、園川（akio-s@ya2.so-net.or.jp）どのによるものです。

その他にもたくさんのパッチをいただきましたが、基本的に山下のオリジナル
です。gn の著作権は山下が保持しています。


再配布について

営利目的の再配布を除いて制限を設けません。
配布を受ける者に対して、再配布のために使用する
	・メディア代金
	・回線料金
	・送料
の実費を越えて請求することを禁止します。

再配布の際のファイル名は可能な限りオリジナルのままにしておいて下さい。
他のネットワークなどへ転載することも営利目的を除いて一切の制限を設けま
せん。なお、転載された場合はその旨連絡いただければ幸いです。

バイナリでの配布も、上記範囲内ならソースでの配布と同じように扱って構い
ません。ただし、配布するバイナリに関して、
・ウィルス（など）が混入していないこと
・ランタイムライブラリも配布可能なこと
を配布者が責任を持って確認して下さい。
また、バイナリにはドメイン名などがハードコートされますから実行時カスタ
マイズが必要になります。詳しくはこのディレクトリにバイナリ配布について
のドキュメント（bin-dist）を置いておきますので参考にして下さい。


FD/CD-ROM への収録について

雑誌や書籍に付録として添付される FD/CD-ROM への収録は問題ありません。
自由におこなっていただいて結構です。ただし、事前にその旨連絡をいただけ
れば幸いです。また、見本誌を山下あて送付いただければなお幸いです。

単体で配布される FD/CD-ROM に関しては上記「再配布について」に準じ、営
利目的の配布を除いて制限を設けません。こちらも事前にその旨連絡いただけ
れば幸いです。


改変について

営利目的の改変を除いて、制限を設けません。
営利目的の商品に組み込み、販売するために gn を改変することは禁止します。

もし、何らかの改変をしようと計画された場合、貴方が実施しようとしている
改変が既に他の方によって実施されている可能性があります。
あらかじめ山下に確認していただければ重複開発がなくなり、貴重な時間を無
駄にすることがなくなるかと思います。

また、改変／移植／不具合情報はできるだけ私宛にフィードバック下さい。
フィードバック下さることにより、次期バージョンに反映することが可能にな
りますし、派生バージョンをなくすることにもなります。是非とも御協力をお
願いします。
＃勝手に改変した gn/gnspool を使用してポストされた記事を見ると、
＃私は不快感をおぼえます。

なお、山下宛に送られた gn に関するパッチ／情報は、デフォルトですべての
権利を山下に委譲されたものとして扱います。委譲できない権利に関しても主
張されないものとして扱います。あらかじめ御了承下さい。


使用について

極端な営利目的の使用を除いて制限を設けません。
＃「極端な営利目的の使用」の例が思い浮かばない、、、

「gn を使って NetNews にアクセスしたため、効率が上がり間接的に利益に結
び付いた」というのは「極端な営利目的の使用」とは考えません。
よって、営利目的団体内での通常の使用に関して制限を設けるものではありま
せん。

もし 「gn 導入ガイド」のようなタイトルの手引書を作られることがありまし
たら、「オムロンソフトウェア（株）の山下（yamasita@omronsoft.co.jp）が
多数の協力を得て作成したフリーソフトウェア」である旨明記いただければ幸
いです。また、その手引書のコピーを１部山下宛に送付いただければありがた
くおもいます。


補償について

gn は、正常に動作することを目標に作成されていますが、
	ポストしたつもりがポストされていなかった
	ファイルに保存したつもりが保存されていなかった
	既読情報が消えてしまった
	マシンがロックした
といったような直接的な損害や、
	ニュースの読み過ぎが原因で睡眠不足になり風邪をひいた :-)
といったような間接的な損害をはじめ、何らかの損失、不利益、不都合が発生
したとしても、山下および山下の所属団体は、一切の補償は致しません。
あらかじめ、御了承下さい。


その他

ここに記述した制限事項は、あくまで原則です。
gn の普及や改善に特に貢献すると考えられる場合は、特例を認める場合があ
ります。個別にご相談ください。
また、不明な点がありましたら、何なりとご質問ください。

謝辞

最後になりましたが、gn に関して、いろいろとアドバイスや移植情報、不具
合情報、修正パッチをいただいた
	日外アソシエーツの秋場（akiba@nichigai.co.jp）さま
	沢田＠ムライ電気（sawada@murai-electric.co.jp）さま
	宅間＠松下ソフトリサーチ（takuma@msr.mei.co.jp）さま
	高橋＠釧路高専（akira@marimo.kushiro-ct.ac.jp）さま
	渡辺＠富士フイルム（nabe@miya.fujifilm.co.jp）さま
	西川＠新日鐵（nisikawa@elelab.nsc.co.jp）
		現 都立大学の西川 （kiyoshi@isys3.eei.metro-u.ac.jp）さま
	応用技術株式会社の河副（kawazoe@ogihp.apptec.co.jp）さま
	大山@慶應大学（ohyama@yy.cs.keio.ac.jp）さま
	渡辺＠佐賀大学（watanabe@himiko.cc.saga-u.ac.jp）さま
	森山＠長崎大学（matsu@undead.cc.nagasaki-u.ac.jp）さま
	長崎大学の鶴（tsuru@siebold.cc.nagasaki-u.ac.jp）さま

	高橋＠東芝（hidetaka@pst01.pw.nasu.toshiba.co.jp）さま
	田付＠三菱電機（tatsuki@nkai.cow.melco.co.jp）さま
	真鍋＠豊技大（manabe@papilio.tutics.tut.ac.jp）さま
	富士通(株)の小林（koba@bss.ts.fujitsu.co.jp）さま
	柴田＠豊橋技術科学大学（shibata@tutrp.tut.ac.jp）さま
	佐藤（友）＠ＮＥＣアイシーマイコンシステム
		（y-sato@nk4cs29.lsi.tmg.nec.co.jp）さま
	大野＠長崎大（ohno@mecsolid.mech.nagasaki-u.ac.jp）さま
	小山@東芝（nori@ssel.toshiba.co.jp）さま
	浜地＠ＰＦＵ（hamaji@sole.trad.pfu.fujitsu.co.jp）さま
	(株)本田技術研究所の中村（nakamura@f14k.wrc.honda.co.jp）さま
	近藤＠埼玉大学（kazumasa@soil.civil.saitama-u.ac.jp）さま
	内田@"NEC Software Kyushu"（uchida@bs2.qnes.nec.co.jp）さま
	国立生理学研究所の水谷（mizutani@nips.ac.jp）さま
	小林＠リコー（koba@cs.ricoh.co.jp）さま
	ＮＥＣの青山（hiro@alice.chis.mt.nec.co.jp）さま
	井上＠ＮＨＫ技研（inouet@strl.nhk.or.jp）さま
	土屋＠三洋電機（seichi@seichi.cmpd.sanyo.co.jp）さま
	長田＠ＰＦＵ（nag@xanadu.trad.pfu.fujitsu.co.jp）さま
	塚本＠群馬大学（tukamoto@cs.gunma-u.ac.jp）さま
	黒江＠大日本印刷（a_kuroe@cdc.dnp.co.jp）さま
	藤松＠横河電機（fuji@ks.yokogawa.co.jp）さま
	市川＠コンピュータ・アプリケーション
		（ichi@ca-server.comp-appl.co.jp）さま

	日本 IBM の菊池（kiku@yamato.ibm.co.jp）さま
	Hideo Irie （michael@sm.sony.co.jp）さま
	日本ディジタルイクイップメント（株）本多
		（Shinjirou.Honda@tko.dec-j.co.jp）さま

	たきかわ＠おれごん（takikawm@chert.CS.ORST.EDU）さま
	三菱電機の垣谷（kakiya@han.cow.melco.co.jp）さま
	カシオ計算機の厚目（atsume@casiogw.rd.casio.co.jp）さま
	高橋＠埼大情報（j119@edu.ics.saitama-u.ac.jp）さま
	（財）横浜市青少年科学普及協会の山田陽志郎（yamada@ysc.go.jp）さま
	NEC コンピュータ事業部 の太田俊哉（oota@pes.com1.fc.nec.co.jp）さま
	湯浅＠早大（MAP3877@mapletown.or.jp）さま
	おーうち＠ＮＥＣソフトウェア四国（jh5qla@d1.comd.snes.nec.co.jp）さま

	山崎＠九大航空（yamasaki@aero.kyushu-u.ac.jp）さま
	三菱電機メカトロニクスソフトウエアの七海
		（nanaumi@mswsap.ina.melco.co.jp）さま
	岩渕＠日立（iwabuc@sdl.hitachi.co.jp）さま
	五十嵐＠富士ゼロックス（igarashi@q.ksp.fujixerox.co.jp）さま

	情報出版システム統括部）システム部）竹元朝人（a-take@tokyo.se.fujitsu.co.jp）さま
	村田＠牧丘町立病院内科（nob@makioka.y-min.or.jp）さま
	福田孝康（t_fukuda@yk.rim.or.jp/fukuda@ams.co.jp）さま

	小峯＠ＮＯＳ（xp7a-ngs@j.asahi-net.or.jp）さま
	中村（Takahiro.Nakamura@Zodiac.IIJnet.or.jp）さま
	堀＠東大機械（toshi@nml.t.u-tokyo.ac.jp）さま
	新膳@明電舎（shinzen@denki.meidensha.co.jp）さま

	Masuda Kenya（masudak@ppp.bekkoame.or.jp）さま
	岡田@KOBE（riotaro@kh.rim.or.jp）さま
	中山＠名大（takeshi@sakabe.nuie.nagoya-u.ac.jp）さま
	新岡（niioka@gem.bekkoame.or.jp）さま
	NTT の田中（tanaka@boris.sl.cae.ntt.jp）さま
	細井@ソニー・テクトロニクス（hosoi@sonytek.co.jp）さま
	堂込＠日本無線(ＪＲＣ)（gome@ca2.so-net.or.jp）さま
	野首（knok@da2.so-net.or.jp）さま
	佐久間（t_sakuma@st.rim.or.jp）さま

	平山＠ＪＶＣ（hirayama@krhm.jvc-victor.co.jp）さま
	峯下（minesita@rd.njk.co.jp）さま

	野村（nom@yk.fujitsu.co.jp）さま
	上野 博（zodiac@ibm.net）さま

	兼松 真哉（kane@lp.nm.fujitsu.co.jp）さま
	金指＠ベッコアメ（sonnar@cap.bekkoame.or.jp）さま

	東京大学工学部の倉橋（kura@melchior.q.t.u-tokyo.ac.jp）さま
	川村@鳥取大学（kawamura@ike.tottori-u.ac.jp）さま
	にしたま＠こまつし（m_nisida@ca2.so-net.ne.jp）さま
をはじめ
	fj.news.reader.gn で情報を頂いた皆様
	gn メイリングリスト各位
	omron.co.jp 各位
	omronsoft.co.jp 各位
には大変感謝しております。

今後ともよろしくお願いします。
                                                     以上
Copyright (C) yamasita@omronsoft.co.jp
	May.13,1998
著作権は放棄しません。ただし、営利目的以外の使用／配布に制限は設けません。
